#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/apic.h>
#include <axiom/arch/interrupts.h>
#include <axiom/arch/io.h>
#include <axiom/drivers/keyboard.h>

#define PS2_DATA_PORT    0x60u
#define PS2_STATUS_PORT  0x64u
#define PS2_COMMAND_PORT 0x64u

#define PS2_STATUS_OUTPUT_FULL 0x01u
#define PS2_STATUS_INPUT_FULL  0x02u
#define PS2_STATUS_AUX_DATA    0x20u

#define PS2_CMD_DISABLE_FIRST  0xADu
#define PS2_CMD_DISABLE_SECOND 0xA7u
#define PS2_CMD_ENABLE_FIRST   0xAEu
#define PS2_CMD_READ_CONFIG   0x20u
#define PS2_CMD_WRITE_CONFIG  0x60u

#define PS2_CONFIG_IRQ1          0x01u
#define PS2_CONFIG_IRQ12         0x02u
#define PS2_CONFIG_FIRST_CLOCK   0x10u
#define PS2_CONFIG_TRANSLATION   0x40u

#define KEYBOARD_CMD_ENABLE_SCANNING 0xF4u
#define KEYBOARD_ACK                 0xFAu
#define KEYBOARD_RESEND              0xFEu

#define PS2_WAIT_LIMIT 1000000u

static volatile char input_buffer[KEYBOARD_BUFFER_CAPACITY];
static volatile size_t buffer_head;
static volatile size_t buffer_tail;

static volatile uint64_t irq_count;
static volatile uint64_t scancode_count;
static volatile uint64_t character_count;
static volatile uint64_t dropped_characters;

static uint8_t left_shift;
static uint8_t right_shift;
static uint8_t caps_lock;
static uint8_t extended_prefix;

static const char normal_map[128] = {
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4',
    [0x06] = '5', [0x07] = '6', [0x08] = '7', [0x09] = '8',
    [0x0A] = '9', [0x0B] = '0', [0x0C] = '-', [0x0D] = '=',
    [0x0E] = '\b', [0x0F] = '\t',
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r',
    [0x14] = 't', [0x15] = 'y', [0x16] = 'u', [0x17] = 'i',
    [0x18] = 'o', [0x19] = 'p', [0x1A] = '[', [0x1B] = ']',
    [0x1C] = '\n',
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f',
    [0x22] = 'g', [0x23] = 'h', [0x24] = 'j', [0x25] = 'k',
    [0x26] = 'l', [0x27] = ';', [0x28] = '\'', [0x29] = '`',
    [0x2B] = '\\',
    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v',
    [0x30] = 'b', [0x31] = 'n', [0x32] = 'm', [0x33] = ',',
    [0x34] = '.', [0x35] = '/', [0x39] = ' '
};

static const char shifted_map[128] = {
    [0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$',
    [0x06] = '%', [0x07] = '^', [0x08] = '&', [0x09] = '*',
    [0x0A] = '(', [0x0B] = ')', [0x0C] = '_', [0x0D] = '+',
    [0x1A] = '{', [0x1B] = '}', [0x27] = ':', [0x28] = '"',
    [0x29] = '~', [0x2B] = '|', [0x33] = '<', [0x34] = '>',
    [0x35] = '?'
};

static int ps2_wait_input_clear(void)
{
    uint32_t remaining = PS2_WAIT_LIMIT;

    while (remaining-- != 0u) {
        if ((inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_FULL) == 0u) {
            return 1;
        }
    }

    return 0;
}

static int ps2_wait_output_full(void)
{
    uint32_t remaining = PS2_WAIT_LIMIT;

    while (remaining-- != 0u) {
        if ((inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) != 0u) {
            return 1;
        }
    }

    return 0;
}

static int ps2_write_command(uint8_t command)
{
    if (!ps2_wait_input_clear()) {
        return 0;
    }

    outb(PS2_COMMAND_PORT, command);
    return 1;
}

static int ps2_write_data(uint8_t data)
{
    if (!ps2_wait_input_clear()) {
        return 0;
    }

    outb(PS2_DATA_PORT, data);
    return 1;
}

static int ps2_read_data(uint8_t *data)
{
    if (data == 0 || !ps2_wait_output_full()) {
        return 0;
    }

    *data = inb(PS2_DATA_PORT);
    return 1;
}

static void ps2_flush_output(void)
{
    uint32_t remaining = 256u;

    while (remaining-- != 0u &&
           (inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) != 0u) {
        (void)inb(PS2_DATA_PORT);
    }
}

static int keyboard_device_command(uint8_t command)
{
    uint8_t response;
    unsigned int attempt;

    for (attempt = 0; attempt < 2u; ++attempt) {
        if (!ps2_write_data(command) || !ps2_read_data(&response)) {
            return 0;
        }

        if (response == KEYBOARD_ACK) {
            return 1;
        }

        if (response != KEYBOARD_RESEND) {
            return 0;
        }
    }

    return 0;
}

static int is_ascii_letter(char character)
{
    return character >= 'a' && character <= 'z';
}

static void enqueue_character(char character)
{
    const size_t next = (buffer_head + 1u) % KEYBOARD_BUFFER_CAPACITY;

    if (next == buffer_tail) {
        ++dropped_characters;
        return;
    }

    input_buffer[buffer_head] = character;
    buffer_head = next;
    ++character_count;
}

static void decode_scancode(uint8_t scancode)
{
    uint8_t code;
    int released;
    int shifted;
    char character;

    ++scancode_count;

    if (scancode == 0xE0u) {
        extended_prefix = 1u;
        return;
    }

    code = scancode & 0x7Fu;
    released = (scancode & 0x80u) != 0u;

    if (extended_prefix != 0u) {
        /*
         * Phase 7 only turns normal text keys into characters. Consume the
         * second byte of an extended set-1 sequence so it cannot be mistaken
         * for an ordinary key. Arrow/navigation keys can be exposed later.
         */
        extended_prefix = 0u;
        return;
    }

    if (code == 0x2Au) {
        left_shift = released ? 0u : 1u;
        return;
    }

    if (code == 0x36u) {
        right_shift = released ? 0u : 1u;
        return;
    }

    if (released) {
        return;
    }

    if (code == 0x3Au) {
        caps_lock ^= 1u;
        return;
    }

    character = normal_map[code];

    if (character == '\0') {
        return;
    }

    shifted = (left_shift != 0u || right_shift != 0u);

    if (is_ascii_letter(character)) {
        if (shifted != (caps_lock != 0u)) {
            character = (char)(character - 'a' + 'A');
        }
    } else if (shifted && shifted_map[code] != '\0') {
        character = shifted_map[code];
    }

    enqueue_character(character);
}

static struct interrupt_frame *keyboard_irq_handler(struct interrupt_frame *frame)
{
    uint8_t status;

    ++irq_count;

    /*
     * A controller may already contain more than one byte when IRQ1 runs.
     * Drain keyboard bytes now, but do not consume second-port (mouse) bytes.
     */
    for (;;) {
        status = inb(PS2_STATUS_PORT);

        if ((status & PS2_STATUS_OUTPUT_FULL) == 0u) {
            break;
        }

        if ((status & PS2_STATUS_AUX_DATA) != 0u) {
            break;
        }

        decode_scancode(inb(PS2_DATA_PORT));
    }

    return frame;
}

int keyboard_init(void)
{
    uint8_t config;

    if (interrupts_enabled()) {
        return 0;
    }

    if (irq_register(1u, keyboard_irq_handler) != 0) {
        return 0;
    }

    if (!ps2_write_command(PS2_CMD_DISABLE_FIRST) ||
        !ps2_write_command(PS2_CMD_DISABLE_SECOND)) {
        return 0;
    }

    ps2_flush_output();

    if (!ps2_write_command(PS2_CMD_READ_CONFIG) ||
        !ps2_read_data(&config)) {
        return 0;
    }

    config |= PS2_CONFIG_IRQ1;
    config &= (uint8_t)~PS2_CONFIG_IRQ12;
    config |= PS2_CONFIG_TRANSLATION;
    config &= (uint8_t)~PS2_CONFIG_FIRST_CLOCK;

    if (!ps2_write_command(PS2_CMD_WRITE_CONFIG) ||
        !ps2_write_data(config) ||
        !ps2_write_command(PS2_CMD_ENABLE_FIRST)) {
        return 0;
    }

    ps2_flush_output();

    if (!keyboard_device_command(KEYBOARD_CMD_ENABLE_SCANNING)) {
        return 0;
    }

    buffer_head = 0u;
    buffer_tail = 0u;
    irq_count = 0u;
    scancode_count = 0u;
    character_count = 0u;
    dropped_characters = 0u;
    left_shift = 0u;
    right_shift = 0u;
    caps_lock = 0u;
    extended_prefix = 0u;

    return apic_route_isa_irq(1u, 33u);
}

int keyboard_read_char(char *character)
{
    size_t tail;

    if (character == 0) {
        return 0;
    }

    tail = buffer_tail;

    if (tail == buffer_head) {
        return 0;
    }

    *character = input_buffer[tail];
    buffer_tail = (tail + 1u) % KEYBOARD_BUFFER_CAPACITY;
    return 1;
}

size_t keyboard_pending(void)
{
    const size_t head = buffer_head;
    const size_t tail = buffer_tail;

    if (head >= tail) {
        return head - tail;
    }

    return KEYBOARD_BUFFER_CAPACITY - tail + head;
}

struct keyboard_stats keyboard_get_stats(void)
{
    const struct keyboard_stats stats = {
        .irq_count = irq_count,
        .scancode_count = scancode_count,
        .character_count = character_count,
        .dropped_characters = dropped_characters,
    };

    return stats;
}
