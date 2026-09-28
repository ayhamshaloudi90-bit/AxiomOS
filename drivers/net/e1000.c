#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/pci.h>
#include <axiom/drivers/e1000.h>
#include <axiom/memory/address.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>

#define E1000_VENDOR_INTEL 0x8086u
#define E1000_DEVICE_82540EM 0x100Eu

#define PCI_COMMAND 0x04u
#define PCI_COMMAND_MEMORY (1u << 1)
#define PCI_COMMAND_BUS_MASTER (1u << 2)
#define PCI_BAR0 0x10u
#define PCI_BAR_IO_SPACE 0x1u
#define PCI_BAR_MEMORY_TYPE_MASK 0x6u
#define PCI_BAR_MEMORY_64 0x4u
#define PCI_BAR_MEMORY_MASK 0xFFFFFFF0u

/* 82540EM exposes a 128 KiB register window. */
#define E1000_MMIO_VA 0xFFFFD00000100000ULL
#define E1000_MMIO_BYTES 0x20000u
#define E1000_MMIO_PAGES (E1000_MMIO_BYTES / VMM_PAGE_SIZE)

#define E1000_REG_CTRL  0x0000u
#define E1000_REG_STATUS 0x0008u
#define E1000_REG_ICR   0x00C0u
#define E1000_REG_IMC   0x00D8u
#define E1000_REG_RCTL  0x0100u
#define E1000_REG_TCTL  0x0400u
#define E1000_REG_TIPG  0x0410u
#define E1000_REG_RDBAL 0x2800u
#define E1000_REG_RDBAH 0x2804u
#define E1000_REG_RDLEN 0x2808u
#define E1000_REG_RDH   0x2810u
#define E1000_REG_RDT   0x2818u
#define E1000_REG_TDBAL 0x3800u
#define E1000_REG_TDBAH 0x3804u
#define E1000_REG_TDLEN 0x3808u
#define E1000_REG_TDH   0x3810u
#define E1000_REG_TDT   0x3818u
#define E1000_REG_MTA   0x5200u
#define E1000_REG_RAL0  0x5400u
#define E1000_REG_RAH0  0x5404u

#define E1000_CTRL_SLU (1u << 6)
#define E1000_RCTL_EN  (1u << 1)
#define E1000_RCTL_BAM (1u << 15)
#define E1000_RCTL_SECRC (1u << 26)
#define E1000_TCTL_EN  (1u << 1)
#define E1000_TCTL_PSP (1u << 3)

#define E1000_RX_STATUS_DD  (1u << 0)
#define E1000_RX_STATUS_EOP (1u << 1)
#define E1000_TX_STATUS_DD  (1u << 0)
#define E1000_TX_CMD_EOP    (1u << 0)
#define E1000_TX_CMD_IFCS   (1u << 1)
#define E1000_TX_CMD_RS     (1u << 3)

#define E1000_RX_COUNT 16u
#define E1000_TX_COUNT 8u
#define E1000_BUFFER_BYTES 2048u
#define E1000_TX_WAIT 5000000u

struct e1000_rx_desc {
    uint64_t address;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed));

struct e1000_tx_desc {
    uint64_t address;
    uint16_t length;
    uint8_t checksum_offset;
    uint8_t command;
    uint8_t status;
    uint8_t checksum_start;
    uint16_t special;
} __attribute__((packed));

_Static_assert(sizeof(struct e1000_rx_desc) == 16u, "E1000 RX descriptor size");
_Static_assert(sizeof(struct e1000_tx_desc) == 16u, "E1000 TX descriptor size");

struct e1000_state {
    volatile uint8_t *mmio;
    uint8_t mac[6];
    paddr_t rx_ring_phys;
    paddr_t tx_ring_phys;
    volatile struct e1000_rx_desc *rx_ring;
    volatile struct e1000_tx_desc *tx_ring;
    paddr_t rx_buffer_phys[E1000_RX_COUNT];
    paddr_t tx_buffer_phys[E1000_TX_COUNT];
    uint8_t *rx_buffers[E1000_RX_COUNT];
    uint8_t *tx_buffers[E1000_TX_COUNT];
    uint32_t rx_index;
    uint32_t tx_index;
    struct e1000_stats stats;
    int initialized;
    int available;
};

static struct e1000_state state;

static void bytes_clear(void *memory, size_t count)
{
    volatile uint8_t *out = (volatile uint8_t *)memory;
    size_t index;
    for (index = 0u; index < count; ++index) out[index] = 0u;
}

static void bytes_copy(void *destination, const void *source, size_t count)
{
    uint8_t *out = (uint8_t *)destination;
    const uint8_t *in = (const uint8_t *)source;
    size_t index;
    for (index = 0u; index < count; ++index) out[index] = in[index];
}

static inline uint32_t mmio_read32(uint32_t offset)
{
    volatile uint32_t *reg = (volatile uint32_t *)(void *)(state.mmio + offset);
    const uint32_t value = *reg;
    __asm__ volatile ("mfence" ::: "memory");
    return value;
}

static inline void mmio_write32(uint32_t offset, uint32_t value)
{
    volatile uint32_t *reg = (volatile uint32_t *)(void *)(state.mmio + offset);
    *reg = value;
    __asm__ volatile ("mfence" ::: "memory");
}

static int map_mmio(paddr_t physical)
{
    uint32_t page;

    physical &= ~(paddr_t)(VMM_PAGE_SIZE - 1u);
    for (page = 0u; page < E1000_MMIO_PAGES; ++page) {
        const vaddr_t va = E1000_MMIO_VA + (vaddr_t)page * VMM_PAGE_SIZE;
        const paddr_t pa = physical + (paddr_t)page * VMM_PAGE_SIZE;
        const paddr_t current = virt_to_phys(va);

        if (current != PADDR_INVALID) {
            if ((current & ~(VMM_PAGE_SIZE - 1ULL)) != pa) return 0;
            continue;
        }
        if (!map_page(
                va,
                pa,
                VMM_FLAG_WRITABLE |
                VMM_FLAG_WRITE_THROUGH |
                VMM_FLAG_CACHE_DISABLE |
                VMM_FLAG_NO_EXECUTE
            )) {
            return 0;
        }
    }
    return 1;
}

static int allocate_dma_page(paddr_t *physical_out, void **virtual_out)
{
    paddr_t physical;
    void *virtual_address;

    if (physical_out == 0 || virtual_out == 0) return 0;
    physical = pmm_alloc_page();
    if (physical == PADDR_INVALID) return 0;
    virtual_address = pmm_phys_to_hhdm(physical);
    if (virtual_address == 0) {
        (void)pmm_free_page(physical);
        return 0;
    }
    bytes_clear(virtual_address, VMM_PAGE_SIZE);
    *physical_out = physical;
    *virtual_out = virtual_address;
    return 1;
}

static int read_mac(void)
{
    const uint32_t low = mmio_read32(E1000_REG_RAL0);
    const uint32_t high = mmio_read32(E1000_REG_RAH0);

    state.mac[0] = (uint8_t)low;
    state.mac[1] = (uint8_t)(low >> 8);
    state.mac[2] = (uint8_t)(low >> 16);
    state.mac[3] = (uint8_t)(low >> 24);
    state.mac[4] = (uint8_t)high;
    state.mac[5] = (uint8_t)(high >> 8);

    return (state.mac[0] | state.mac[1] | state.mac[2] |
            state.mac[3] | state.mac[4] | state.mac[5]) != 0u;
}

static int setup_rx(void)
{
    void *ring_memory;
    uint32_t index;

    if (!allocate_dma_page(&state.rx_ring_phys, &ring_memory)) return 0;
    state.rx_ring = (volatile struct e1000_rx_desc *)ring_memory;

    for (index = 0u; index < E1000_RX_COUNT; ++index) {
        void *buffer;
        if (!allocate_dma_page(&state.rx_buffer_phys[index], &buffer)) return 0;
        state.rx_buffers[index] = (uint8_t *)buffer;
        state.rx_ring[index].address = state.rx_buffer_phys[index];
        state.rx_ring[index].status = 0u;
    }

    mmio_write32(E1000_REG_RDBAL, (uint32_t)state.rx_ring_phys);
    mmio_write32(E1000_REG_RDBAH, (uint32_t)(state.rx_ring_phys >> 32));
    mmio_write32(E1000_REG_RDLEN, E1000_RX_COUNT * sizeof(struct e1000_rx_desc));
    mmio_write32(E1000_REG_RDH, 0u);
    mmio_write32(E1000_REG_RDT, E1000_RX_COUNT - 1u);
    state.rx_index = 0u;

    mmio_write32(E1000_REG_RCTL, E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC);
    return 1;
}

static int setup_tx(void)
{
    void *ring_memory;
    uint32_t index;

    if (!allocate_dma_page(&state.tx_ring_phys, &ring_memory)) return 0;
    state.tx_ring = (volatile struct e1000_tx_desc *)ring_memory;

    for (index = 0u; index < E1000_TX_COUNT; ++index) {
        void *buffer;
        if (!allocate_dma_page(&state.tx_buffer_phys[index], &buffer)) return 0;
        state.tx_buffers[index] = (uint8_t *)buffer;
        state.tx_ring[index].address = state.tx_buffer_phys[index];
        state.tx_ring[index].status = E1000_TX_STATUS_DD;
    }

    mmio_write32(E1000_REG_TDBAL, (uint32_t)state.tx_ring_phys);
    mmio_write32(E1000_REG_TDBAH, (uint32_t)(state.tx_ring_phys >> 32));
    mmio_write32(E1000_REG_TDLEN, E1000_TX_COUNT * sizeof(struct e1000_tx_desc));
    mmio_write32(E1000_REG_TDH, 0u);
    mmio_write32(E1000_REG_TDT, 0u);
    state.tx_index = 0u;

    mmio_write32(E1000_REG_TIPG, 0x0060200Au);
    mmio_write32(
        E1000_REG_TCTL,
        E1000_TCTL_EN | E1000_TCTL_PSP | (0x10u << 4) | (0x40u << 12)
    );
    return 1;
}

int e1000_init(void)
{
    struct pci_device device;
    uint32_t bar_low;
    uint64_t bar_address;
    uint16_t command;
    uint32_t mta_index;

    if (state.initialized) return state.available;
    state.initialized = 1;

    if (!pci_find_device(E1000_VENDOR_INTEL, E1000_DEVICE_82540EM, &device)) {
        return 0;
    }

    bar_low = pci_config_read32(device.bus, device.device, device.function, PCI_BAR0);
    if ((bar_low & PCI_BAR_IO_SPACE) != 0u) return 0;

    bar_address = (uint64_t)(bar_low & PCI_BAR_MEMORY_MASK);
    if ((bar_low & PCI_BAR_MEMORY_TYPE_MASK) == PCI_BAR_MEMORY_64) {
        const uint32_t high = pci_config_read32(
            device.bus, device.device, device.function, PCI_BAR0 + 4u
        );
        bar_address |= (uint64_t)high << 32;
    }
    if (bar_address == 0u || !map_mmio((paddr_t)bar_address)) return 0;

    command = pci_config_read16(device.bus, device.device, device.function, PCI_COMMAND);
    command |= PCI_COMMAND_MEMORY | PCI_COMMAND_BUS_MASTER;
    pci_config_write16(device.bus, device.device, device.function, PCI_COMMAND, command);

    state.mmio = (volatile uint8_t *)(uintptr_t)E1000_MMIO_VA;

    /* This phase is intentionally polled. Mask all NIC interrupts. */
    mmio_write32(E1000_REG_IMC, 0xFFFFFFFFu);
    (void)mmio_read32(E1000_REG_ICR);
    mmio_write32(E1000_REG_CTRL, mmio_read32(E1000_REG_CTRL) | E1000_CTRL_SLU);

    for (mta_index = 0u; mta_index < 128u; ++mta_index) {
        mmio_write32(E1000_REG_MTA + mta_index * 4u, 0u);
    }

    if (!read_mac() || !setup_rx() || !setup_tx()) return 0;

    state.available = 1;
    return 1;
}

int e1000_available(void)
{
    return state.available;
}

int e1000_send(const void *frame, size_t length)
{
    volatile struct e1000_tx_desc *descriptor;
    uint32_t remaining;
    const uint32_t index = state.tx_index;

    if (!state.available || frame == 0 || length < 14u ||
        length > E1000_ETHERNET_FRAME_MAX) {
        return 0;
    }

    descriptor = &state.tx_ring[index];
    remaining = E1000_TX_WAIT;
    while ((descriptor->status & E1000_TX_STATUS_DD) == 0u) {
        if (remaining-- == 0u) {
            ++state.stats.tx_errors;
            return 0;
        }
        __asm__ volatile ("pause");
    }

    bytes_copy(state.tx_buffers[index], frame, length);
    descriptor->length = (uint16_t)length;
    descriptor->checksum_offset = 0u;
    descriptor->checksum_start = 0u;
    descriptor->special = 0u;
    descriptor->status = 0u;
    descriptor->command = E1000_TX_CMD_EOP | E1000_TX_CMD_IFCS | E1000_TX_CMD_RS;
    __asm__ volatile ("mfence" ::: "memory");

    state.tx_index = (index + 1u) % E1000_TX_COUNT;
    mmio_write32(E1000_REG_TDT, state.tx_index);

    remaining = E1000_TX_WAIT;
    while ((descriptor->status & E1000_TX_STATUS_DD) == 0u) {
        if (remaining-- == 0u) {
            ++state.stats.tx_errors;
            return 0;
        }
        __asm__ volatile ("pause");
    }

    ++state.stats.tx_frames;
    state.stats.tx_bytes += length;
    return 1;
}

int e1000_receive(void *frame, size_t capacity, size_t *length_out)
{
    volatile struct e1000_rx_desc *descriptor;
    size_t length;
    uint32_t index;

    if (!state.available || frame == 0 || length_out == 0) return 0;

    index = state.rx_index;
    descriptor = &state.rx_ring[index];
    __asm__ volatile ("mfence" ::: "memory");
    if ((descriptor->status & E1000_RX_STATUS_DD) == 0u) return 0;

    length = descriptor->length;
    if ((descriptor->status & E1000_RX_STATUS_EOP) == 0u ||
        descriptor->errors != 0u || length > capacity ||
        length > E1000_BUFFER_BYTES) {
        ++state.stats.rx_dropped;
        length = 0u;
    } else {
        bytes_copy(frame, state.rx_buffers[index], length);
        ++state.stats.rx_frames;
        state.stats.rx_bytes += length;
    }

    descriptor->status = 0u;
    descriptor->errors = 0u;
    descriptor->length = 0u;
    __asm__ volatile ("mfence" ::: "memory");
    mmio_write32(E1000_REG_RDT, index);
    state.rx_index = (index + 1u) % E1000_RX_COUNT;

    *length_out = length;
    return length != 0u;
}

void e1000_mac(uint8_t mac_out[6])
{
    uint32_t index;
    if (mac_out == 0) return;
    for (index = 0u; index < 6u; ++index) mac_out[index] = state.mac[index];
}

struct e1000_stats e1000_get_stats(void)
{
    return state.stats;
}
