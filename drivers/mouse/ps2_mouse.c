#include <stdint.h>

#include <axiom/arch/apic.h>
#include <axiom/arch/interrupts.h>
#include <axiom/arch/io.h>
#include <axiom/drivers/mouse.h>

#define PS2_DATA_PORT       0x60u
#define PS2_STATUS_PORT     0x64u
#define PS2_COMMAND_PORT    0x64u

#define PS2_STATUS_OUTPUT_FULL 0x01u
#define PS2_STATUS_INPUT_FULL  0x02u
#define PS2_STATUS_AUX_DATA    0x20u

#define PS2_CMD_ENABLE_SECOND  0xA8u
#define PS2_CMD_READ_CONFIG    0x20u
#define PS2_CMD_WRITE_CONFIG   0x60u
#define PS2_CMD_WRITE_SECOND   0xD4u

#define PS2_CONFIG_IRQ12        0x02u
#define PS2_CONFIG_SECOND_CLOCK 0x20u

#define MOUSE_CMD_SET_DEFAULTS     0xF6u
#define MOUSE_CMD_ENABLE_REPORTING 0xF4u
#define MOUSE_ACK                  0xFAu
#define MOUSE_RESEND               0xFEu

#define PS2_WAIT_LIMIT 1000000u

static volatile int64_t cumulative_x;
static volatile int64_t cumulative_y;
static volatile uint64_t irq_count;
static volatile uint64_t packet_count;
static volatile uint8_t buttons;
static volatile uint8_t available;

static uint8_t packet[3];
static uint8_t packet_index;

static int wait_input_clear(void)
{
    uint32_t remaining = PS2_WAIT_LIMIT;
    while (remaining-- != 0u) {
        if ((inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_FULL) == 0u) return 1;
    }
    return 0;
}

static int wait_output_full(void)
{
    uint32_t remaining = PS2_WAIT_LIMIT;
    while (remaining-- != 0u) {
        if ((inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) != 0u) return 1;
    }
    return 0;
}

static int write_command(uint8_t command)
{
    if (!wait_input_clear()) return 0;
    outb(PS2_COMMAND_PORT, command);
    return 1;
}

static int write_data(uint8_t data)
{
    if (!wait_input_clear()) return 0;
    outb(PS2_DATA_PORT, data);
    return 1;
}

static int read_data(uint8_t *value)
{
    if (value == 0 || !wait_output_full()) return 0;
    *value = inb(PS2_DATA_PORT);
    return 1;
}

static void flush_output(void)
{
    uint32_t remaining = 256u;
    while (remaining-- != 0u &&
           (inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) != 0u) {
        (void)inb(PS2_DATA_PORT);
    }
}

static int mouse_command(uint8_t command)
{
    unsigned attempt;

    for (attempt = 0u; attempt < 3u; ++attempt) {
        uint8_t response;
        uint32_t remaining;

        if (!write_command(PS2_CMD_WRITE_SECOND) || !write_data(command)) {
            return 0;
        }

        remaining = PS2_WAIT_LIMIT;
        while (remaining-- != 0u) {
            const uint8_t status = inb(PS2_STATUS_PORT);
            if ((status & PS2_STATUS_OUTPUT_FULL) == 0u) continue;
            response = inb(PS2_DATA_PORT);
            if ((status & PS2_STATUS_AUX_DATA) == 0u) continue;

            if (response == MOUSE_ACK) return 1;
            if (response == MOUSE_RESEND) break;
            return 0;
        }
    }

    return 0;
}

static void consume_mouse_byte(uint8_t value)
{
    if (packet_index == 0u && (value & 0x08u) == 0u) {
        return;
    }

    packet[packet_index++] = value;
    if (packet_index != 3u) return;
    packet_index = 0u;

    buttons = packet[0] & 0x07u;

    /* Ignore movement when the device reports X/Y overflow. */
    if ((packet[0] & 0xC0u) == 0u) {
        const int8_t dx = (int8_t)packet[1];
        const int8_t dy = (int8_t)packet[2];
        cumulative_x += (int64_t)dx;
        cumulative_y -= (int64_t)dy; /* Screen coordinates grow downward. */
    }

    ++packet_count;
}

static struct interrupt_frame *mouse_irq_handler(struct interrupt_frame *frame)
{
    ++irq_count;

    for (;;) {
        const uint8_t status = inb(PS2_STATUS_PORT);
        if ((status & PS2_STATUS_OUTPUT_FULL) == 0u) break;
        if ((status & PS2_STATUS_AUX_DATA) == 0u) break;
        consume_mouse_byte(inb(PS2_DATA_PORT));
    }

    return frame;
}

int mouse_init(void)
{
    uint8_t config;

    if (interrupts_enabled()) return 0;
    if (irq_register(12u, mouse_irq_handler) != 0) return 0;

    /* Drop stale controller bytes before talking to the second port. */
    flush_output();

    if (!write_command(PS2_CMD_ENABLE_SECOND) ||
        !write_command(PS2_CMD_READ_CONFIG) ||
        !read_data(&config)) {
        (void)apic_mask_isa_irq(12u);
        return 0;
    }

    config |= PS2_CONFIG_IRQ12;
    config &= (uint8_t)~PS2_CONFIG_SECOND_CLOCK;

    if (!write_command(PS2_CMD_WRITE_CONFIG) || !write_data(config)) {
        (void)apic_mask_isa_irq(12u);
        return 0;
    }

    if (!mouse_command(MOUSE_CMD_SET_DEFAULTS) ||
        !mouse_command(MOUSE_CMD_ENABLE_REPORTING)) {
        (void)apic_mask_isa_irq(12u);
        return 0;
    }

    cumulative_x = 0;
    cumulative_y = 0;
    irq_count = 0u;
    packet_count = 0u;
    buttons = 0u;
    packet_index = 0u;
    available = 1u;

    if (!apic_route_isa_irq(12u, 44u)) {
        available = 0u;
        return 0;
    }

    return 1;
}

int mouse_available(void)
{
    return available != 0u;
}

struct mouse_state mouse_get_state(void)
{
    const struct mouse_state state = {
        .x = cumulative_x,
        .y = cumulative_y,
        .irq_count = irq_count,
        .packet_count = packet_count,
        .buttons = buttons,
        .available = available,
    };
    return state;
}
