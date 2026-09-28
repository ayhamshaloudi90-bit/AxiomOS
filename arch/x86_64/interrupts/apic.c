#include <stdint.h>

#include <axiom/arch/apic.h>
#include <axiom/arch/io.h>
#include <axiom/memory/address.h>
#include <axiom/memory/vmm.h>

#define IA32_APIC_BASE_MSR 0x1Bu
#define IA32_APIC_BASE_ENABLE (1ULL << 11)
#define IA32_APIC_BASE_X2APIC (1ULL << 10)
#define IA32_APIC_BASE_MASK   0x000FFFFFFFFFF000ULL

#define LAPIC_MMIO_VA  0xFFFFD00000000000ULL
#define IOAPIC_MMIO_VA 0xFFFFD00000001000ULL

#define Q35_IOAPIC_PHYSICAL 0xFEC00000ULL

#define LAPIC_REG_ID                 0x020u
#define LAPIC_REG_EOI                0x0B0u
#define LAPIC_REG_SPURIOUS           0x0F0u
#define LAPIC_REG_LVT_TIMER          0x320u
#define LAPIC_REG_TIMER_INITIAL      0x380u
#define LAPIC_REG_TIMER_CURRENT      0x390u
#define LAPIC_REG_TIMER_DIVIDE       0x3E0u

#define LAPIC_SOFTWARE_ENABLE        (1u << 8)
#define LAPIC_TIMER_MASKED           (1u << 16)
#define LAPIC_TIMER_PERIODIC         (1u << 17)
#define LAPIC_SPURIOUS_VECTOR        0xFFu
#define LAPIC_TIMER_DIVIDE_BY_16     0x3u

#define IOAPIC_REG_ID                0x00u
#define IOAPIC_REG_VERSION           0x01u
#define IOAPIC_REG_REDIR_BASE        0x10u
#define IOAPIC_REDIR_MASKED          (1u << 16)

#define PIT_CHANNEL2_DATA            0x42u
#define PIT_COMMAND                  0x43u
#define PIT_SPEAKER_CONTROL          0x61u
#define PIT_INPUT_HZ                 1193182u
#define PIT_CHANNEL2_ONESHOT         0xB0u
#define PIT_SPEAKER_GATE2            0x01u
#define PIT_SPEAKER_ENABLE           0x02u
#define PIT_SPEAKER_OUT2             0x20u

#define APIC_CALIBRATION_HZ          100u
#define PIT_POLL_LIMIT               10000000u

static volatile uint32_t *lapic;
static volatile uint32_t *ioapic_select;
static volatile uint32_t *ioapic_window;
static uint8_t local_id;
static uint8_t ioapic_max_redirection;
static uint32_t timer_frequency_hz;
static int initialized;

static uint64_t read_msr(uint32_t msr)
{
    uint32_t low;
    uint32_t high;

    __asm__ volatile (
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(msr)
    );

    return ((uint64_t)high << 32) | low;
}

static void write_msr(uint32_t msr, uint64_t value)
{
    __asm__ volatile (
        "wrmsr"
        :
        : "c"(msr), "a"((uint32_t)value), "d"((uint32_t)(value >> 32))
        : "memory"
    );
}

static int cpu_has_apic(void)
{
    uint32_t eax = 1u;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;

    __asm__ volatile (
        "cpuid"
        : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        :
    );

    (void)ebx;
    (void)ecx;
    return (edx & (1u << 9)) != 0u;
}

static uint32_t lapic_read(uint32_t offset)
{
    return lapic[offset / sizeof(uint32_t)];
}

static void lapic_write(uint32_t offset, uint32_t value)
{
    lapic[offset / sizeof(uint32_t)] = value;
    (void)lapic[offset / sizeof(uint32_t)];
}

static uint32_t ioapic_read(uint8_t reg)
{
    *ioapic_select = reg;
    return *ioapic_window;
}

static void ioapic_write(uint8_t reg, uint32_t value)
{
    *ioapic_select = reg;
    *ioapic_window = value;
}

static int map_mmio_page(vaddr_t virtual_address, paddr_t physical_address)
{
    paddr_t current = virt_to_phys(virtual_address);

    if (current != PADDR_INVALID) {
        return (current & ~(VMM_PAGE_SIZE - 1ULL)) == physical_address;
    }

    return map_page(
        virtual_address,
        physical_address,
        VMM_FLAG_WRITABLE |
        VMM_FLAG_WRITE_THROUGH |
        VMM_FLAG_CACHE_DISABLE |
        VMM_FLAG_NO_EXECUTE
    );
}

static int pit_wait_10ms(void)
{
    const uint16_t count = (uint16_t)(PIT_INPUT_HZ / APIC_CALIBRATION_HZ);
    const uint8_t original = inb(PIT_SPEAKER_CONTROL);
    uint32_t remaining = PIT_POLL_LIMIT;

    /* Gate channel 2 off while loading a fresh mode-0 one-shot count. */
    outb(
        PIT_SPEAKER_CONTROL,
        (uint8_t)(original & ~(PIT_SPEAKER_GATE2 | PIT_SPEAKER_ENABLE))
    );

    outb(PIT_COMMAND, PIT_CHANNEL2_ONESHOT);
    outb(PIT_CHANNEL2_DATA, (uint8_t)(count & 0xFFu));
    outb(PIT_CHANNEL2_DATA, (uint8_t)(count >> 8));

    /* Gate high starts the countdown; leave the speaker itself disabled. */
    outb(
        PIT_SPEAKER_CONTROL,
        (uint8_t)((original & ~PIT_SPEAKER_ENABLE) | PIT_SPEAKER_GATE2)
    );

    while ((inb(PIT_SPEAKER_CONTROL) & PIT_SPEAKER_OUT2) == 0u) {
        if (remaining-- == 0u) {
            outb(PIT_SPEAKER_CONTROL, original);
            return 0;
        }

        __asm__ volatile ("pause");
    }

    outb(PIT_SPEAKER_CONTROL, original);
    return 1;
}

int apic_init(void)
{
    uint64_t apic_base;
    paddr_t lapic_physical;
    uint32_t version;
    uint32_t index;

    if (initialized) {
        return 1;
    }

    if (!cpu_has_apic()) {
        return 0;
    }

    apic_base = read_msr(IA32_APIC_BASE_MSR);

    /* Phase 7 deliberately uses the xAPIC MMIO interface, not x2APIC. */
    if ((apic_base & IA32_APIC_BASE_X2APIC) != 0ULL) {
        return 0;
    }

    if ((apic_base & IA32_APIC_BASE_ENABLE) == 0ULL) {
        apic_base |= IA32_APIC_BASE_ENABLE;
        write_msr(IA32_APIC_BASE_MSR, apic_base);
    }

    lapic_physical = apic_base & IA32_APIC_BASE_MASK;

    if (!map_mmio_page(LAPIC_MMIO_VA, lapic_physical) ||
        !map_mmio_page(IOAPIC_MMIO_VA, Q35_IOAPIC_PHYSICAL)) {
        return 0;
    }

    lapic = (volatile uint32_t *)(uintptr_t)LAPIC_MMIO_VA;
    ioapic_select = (volatile uint32_t *)(uintptr_t)IOAPIC_MMIO_VA;
    ioapic_window = (volatile uint32_t *)(uintptr_t)(IOAPIC_MMIO_VA + 0x10u);

    local_id = (uint8_t)(lapic_read(LAPIC_REG_ID) >> 24);
    version = ioapic_read(IOAPIC_REG_VERSION);
    ioapic_max_redirection = (uint8_t)((version >> 16) & 0xFFu);

    /* Mask every I/O APIC input until a driver explicitly claims it. */
    for (index = 0u; index <= ioapic_max_redirection; ++index) {
        const uint8_t low = (uint8_t)(IOAPIC_REG_REDIR_BASE + index * 2u);
        const uint8_t high = (uint8_t)(low + 1u);

        ioapic_write(high, 0u);
        ioapic_write(low, IOAPIC_REDIR_MASKED);
    }

    /* Enable the local APIC and reserve vector 0xFF for spurious interrupts. */
    lapic_write(
        LAPIC_REG_SPURIOUS,
        LAPIC_SOFTWARE_ENABLE | LAPIC_SPURIOUS_VECTOR
    );

    /* Mask the local APIC timer until timer_init() calibrates it. */
    lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_TIMER_MASKED | 32u);
    lapic_write(LAPIC_REG_TIMER_INITIAL, 0u);

    /* Reading the IOAPIC ID proves the MMIO window responds. */
    (void)ioapic_read(IOAPIC_REG_ID);

    initialized = 1;
    return 1;
}

int apic_active(void)
{
    return initialized;
}

uint8_t apic_local_id(void)
{
    return local_id;
}

int apic_route_isa_irq(uint8_t irq, uint8_t vector)
{
    uint8_t low;
    uint8_t high;

    if (!initialized || irq > ioapic_max_redirection || vector < 32u) {
        return 0;
    }

    low = (uint8_t)(IOAPIC_REG_REDIR_BASE + irq * 2u);
    high = (uint8_t)(low + 1u);

    /* Physical destination, fixed delivery, edge-triggered, active-high. */
    ioapic_write(high, (uint32_t)local_id << 24);
    ioapic_write(low, vector);
    return 1;
}

int apic_mask_isa_irq(uint8_t irq)
{
    uint8_t low;
    uint32_t value;

    if (!initialized || irq > ioapic_max_redirection) {
        return 0;
    }

    low = (uint8_t)(IOAPIC_REG_REDIR_BASE + irq * 2u);
    value = ioapic_read(low);
    ioapic_write(low, value | IOAPIC_REDIR_MASKED);
    return 1;
}

void apic_eoi(void)
{
    if (initialized) {
        lapic_write(LAPIC_REG_EOI, 0u);
    }
}

int apic_timer_start(uint8_t vector, uint32_t frequency_hz)
{
    uint32_t current;
    uint32_t elapsed_10ms;
    uint64_t initial_count;

    if (!initialized || frequency_hz == 0u || vector < 32u) {
        return 0;
    }

    lapic_write(LAPIC_REG_TIMER_DIVIDE, LAPIC_TIMER_DIVIDE_BY_16);
    lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_TIMER_MASKED | vector);
    lapic_write(LAPIC_REG_TIMER_INITIAL, 0xFFFFFFFFu);

    if (!pit_wait_10ms()) {
        lapic_write(LAPIC_REG_TIMER_INITIAL, 0u);
        return 0;
    }

    current = lapic_read(LAPIC_REG_TIMER_CURRENT);
    elapsed_10ms = 0xFFFFFFFFu - current;

    if (elapsed_10ms < 100u) {
        lapic_write(LAPIC_REG_TIMER_INITIAL, 0u);
        return 0;
    }

    /*
     * elapsed_10ms is the LAPIC count for 1/100 second. Scale that to the
     * requested period. Phase 7 requests exactly 100 Hz.
     */
    initial_count =
        ((uint64_t)elapsed_10ms * APIC_CALIBRATION_HZ) /
        frequency_hz;

    if (initial_count == 0ULL || initial_count > UINT32_MAX) {
        lapic_write(LAPIC_REG_TIMER_INITIAL, 0u);
        return 0;
    }

    timer_frequency_hz = frequency_hz;
    lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_TIMER_PERIODIC | vector);
    lapic_write(LAPIC_REG_TIMER_INITIAL, (uint32_t)initial_count);
    return 1;
}

uint32_t apic_timer_frequency(void)
{
    return timer_frequency_hz;
}
