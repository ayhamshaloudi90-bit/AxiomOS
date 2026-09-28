#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/interrupts.h>
#include <axiom/arch/pci.h>
#include <axiom/drivers/ahci.h>
#include <axiom/memory/address.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>

#define PCI_CLASS_MASS_STORAGE 0x01u
#define PCI_SUBCLASS_SATA      0x06u
#define PCI_PROGIF_AHCI        0x01u
#define PCI_COMMAND            0x04u
#define PCI_COMMAND_MEMORY     (1u << 1)
#define PCI_COMMAND_BUS_MASTER (1u << 2)
#define PCI_BAR5               0x24u
#define PCI_BAR_MEMORY_MASK    0xFFFFFFF0u

#define AHCI_MMIO_VA 0xFFFFD00000002000ULL
#define AHCI_MMIO_PAGES 2u

#define HBA_GHC_AE (1u << 31)
#define HBA_CAP2_BOH (1u << 0)
#define HBA_BOHC_BOS (1u << 0)
#define HBA_BOHC_OOS (1u << 1)
#define HBA_BOHC_BB  (1u << 4)
#define HBA_PxCMD_ST  (1u << 0)
#define HBA_PxCMD_FRE (1u << 4)
#define HBA_PxCMD_FR  (1u << 14)
#define HBA_PxCMD_CR  (1u << 15)
#define HBA_PxIS_TFES (1u << 30)
#define HBA_SSTS_DET_MASK 0x0Fu
#define HBA_SSTS_IPM_MASK 0x0F00u
#define HBA_SSTS_DET_PRESENT 0x03u
#define HBA_SSTS_IPM_ACTIVE  0x0100u
#define HBA_SIG_ATA 0x00000101u

#define ATA_CMD_READ_DMA_EXT  0x25u
#define ATA_CMD_WRITE_DMA_EXT 0x35u
#define ATA_CMD_FLUSH_CACHE_EXT 0xEAu
#define ATA_CMD_IDENTIFY      0xECu
#define FIS_TYPE_REG_H2D      0x27u
#define AHCI_CMD_TIMEOUT      20000000u

struct hba_port {
    volatile uint32_t clb;
    volatile uint32_t clbu;
    volatile uint32_t fb;
    volatile uint32_t fbu;
    volatile uint32_t is;
    volatile uint32_t ie;
    volatile uint32_t cmd;
    volatile uint32_t reserved0;
    volatile uint32_t tfd;
    volatile uint32_t sig;
    volatile uint32_t ssts;
    volatile uint32_t sctl;
    volatile uint32_t serr;
    volatile uint32_t sact;
    volatile uint32_t ci;
    volatile uint32_t sntf;
    volatile uint32_t fbs;
    volatile uint32_t reserved1[11];
    volatile uint32_t vendor[4];
};

struct hba_memory {
    volatile uint32_t cap;
    volatile uint32_t ghc;
    volatile uint32_t is;
    volatile uint32_t pi;
    volatile uint32_t vs;
    volatile uint32_t ccc_ctl;
    volatile uint32_t ccc_pts;
    volatile uint32_t em_loc;
    volatile uint32_t em_ctl;
    volatile uint32_t cap2;
    volatile uint32_t bohc;
    uint8_t reserved[0xA0 - 0x2C];
    uint8_t vendor[0x100 - 0xA0];
    struct hba_port ports[32];
};

struct ahci_command_header {
    uint16_t flags;
    uint16_t prdt_length;
    volatile uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t reserved[4];
};

struct ahci_prdt_entry {
    uint32_t dba;
    uint32_t dbau;
    uint32_t reserved;
    uint32_t dbc_i;
};

struct ahci_command_table {
    uint8_t cfis[64];
    uint8_t acmd[16];
    uint8_t reserved[48];
    struct ahci_prdt_entry prdt[1];
};

struct fis_reg_h2d {
    uint8_t fis_type;
    uint8_t pmport_c;
    uint8_t command;
    uint8_t feature_low;
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t feature_high;
    uint8_t count_low;
    uint8_t count_high;
    uint8_t icc;
    uint8_t control;
    uint8_t reserved[4];
};

struct ahci_state {
    struct block_device block;
    struct hba_memory *hba;
    struct hba_port *port;
    uint8_t port_number;
    paddr_t command_list_phys;
    paddr_t fis_phys;
    paddr_t command_table_phys;
    paddr_t bounce_phys;
    struct ahci_command_header *command_list;
    struct ahci_command_table *command_table;
    uint8_t *bounce;
};

static struct ahci_state state;
static int initialized;

static void clear_bytes(void *memory, size_t count)
{
    uint8_t *bytes = (uint8_t *)memory;
    size_t index;
    for (index = 0u; index < count; ++index) {
        bytes[index] = 0u;
    }
}

static void copy_bytes(void *destination, const void *source, size_t count)
{
    uint8_t *out = (uint8_t *)destination;
    const uint8_t *in = (const uint8_t *)source;
    size_t index;
    for (index = 0u; index < count; ++index) {
        out[index] = in[index];
    }
}

static int map_ahci_mmio(paddr_t physical)
{
    uint32_t page;

    for (page = 0u; page < AHCI_MMIO_PAGES; ++page) {
        const vaddr_t va = AHCI_MMIO_VA + (vaddr_t)page * VMM_PAGE_SIZE;
        const paddr_t pa = physical + (paddr_t)page * VMM_PAGE_SIZE;
        const paddr_t current = virt_to_phys(va);

        if (current != PADDR_INVALID) {
            if ((current & ~(VMM_PAGE_SIZE - 1ULL)) != pa) {
                return 0;
            }
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

static int stop_port(struct hba_port *port)
{
    uint32_t remaining = AHCI_CMD_TIMEOUT;

    port->cmd &= ~HBA_PxCMD_ST;
    port->cmd &= ~HBA_PxCMD_FRE;

    while ((port->cmd & (HBA_PxCMD_FR | HBA_PxCMD_CR)) != 0u) {
        if (remaining-- == 0u) {
            return 0;
        }
        __asm__ volatile ("pause");
    }
    return 1;
}

static void start_port(struct hba_port *port)
{
    port->cmd |= HBA_PxCMD_FRE;
    port->cmd |= HBA_PxCMD_ST;
}

static int wait_port_ready(struct hba_port *port)
{
    uint32_t remaining = AHCI_CMD_TIMEOUT;
    while ((port->tfd & (0x80u | 0x08u)) != 0u) {
        if (remaining-- == 0u) {
            return 0;
        }
        __asm__ volatile ("pause");
    }
    return 1;
}

static int issue_command(
    uint8_t command,
    uint64_t lba,
    uint16_t sectors,
    int write,
    int use_data
)
{
    struct ahci_command_header *header = &state.command_list[0];
    struct ahci_command_table *table = state.command_table;
    struct fis_reg_h2d *fis;
    uint32_t remaining;
    int restore_interrupts = interrupts_enabled();

    if (restore_interrupts) {
        interrupts_disable();
    }

    if (!wait_port_ready(state.port)) {
        if (restore_interrupts) {
            interrupts_enable();
        }
        return 0;
    }

    clear_bytes(header, sizeof(*header));
    clear_bytes(table, sizeof(*table));

    header->flags = 5u | (write ? (1u << 6) : 0u);
    header->prdt_length = use_data ? 1u : 0u;
    header->ctba = (uint32_t)state.command_table_phys;
    header->ctbau = (uint32_t)(state.command_table_phys >> 32);

    if (use_data) {
        table->prdt[0].dba = (uint32_t)state.bounce_phys;
        table->prdt[0].dbau = (uint32_t)(state.bounce_phys >> 32);
        table->prdt[0].dbc_i = (BLOCK_SECTOR_SIZE - 1u);
    }

    fis = (struct fis_reg_h2d *)(void *)table->cfis;
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->pmport_c = 1u << 7;
    fis->command = command;
    fis->device = (command == ATA_CMD_READ_DMA_EXT ||
                   command == ATA_CMD_WRITE_DMA_EXT) ? (1u << 6) : 0u;
    fis->lba0 = (uint8_t)lba;
    fis->lba1 = (uint8_t)(lba >> 8);
    fis->lba2 = (uint8_t)(lba >> 16);
    fis->lba3 = (uint8_t)(lba >> 24);
    fis->lba4 = (uint8_t)(lba >> 32);
    fis->lba5 = (uint8_t)(lba >> 40);
    fis->count_low = (uint8_t)sectors;
    fis->count_high = (uint8_t)(sectors >> 8);

    state.port->is = 0xFFFFFFFFu;
    __asm__ volatile ("mfence" ::: "memory");
    state.port->ci = 1u;

    remaining = AHCI_CMD_TIMEOUT;
    while ((state.port->ci & 1u) != 0u) {
        if ((state.port->is & HBA_PxIS_TFES) != 0u || remaining-- == 0u) {
            if (restore_interrupts) {
                interrupts_enable();
            }
            return 0;
        }
        __asm__ volatile ("pause");
    }

    __asm__ volatile ("mfence" ::: "memory");
    if ((state.port->is & HBA_PxIS_TFES) != 0u) {
        if (restore_interrupts) {
            interrupts_enable();
        }
        return 0;
    }

    if (restore_interrupts) {
        interrupts_enable();
    }
    return 1;
}

static int ahci_read_impl(
    struct block_device *device,
    uint64_t lba,
    uint32_t sectors,
    void *buffer
)
{
    uint8_t *out = (uint8_t *)buffer;
    uint32_t index;
    (void)device;

    for (index = 0u; index < sectors; ++index) {
        if (!issue_command(ATA_CMD_READ_DMA_EXT, lba + index, 1u, 0, 1)) {
            return 0;
        }
        copy_bytes(out + (size_t)index * BLOCK_SECTOR_SIZE,
                   state.bounce, BLOCK_SECTOR_SIZE);
    }
    return 1;
}

static int ahci_write_impl(
    struct block_device *device,
    uint64_t lba,
    uint32_t sectors,
    const void *buffer
)
{
    const uint8_t *in = (const uint8_t *)buffer;
    uint32_t index;
    (void)device;

    for (index = 0u; index < sectors; ++index) {
        copy_bytes(state.bounce,
                   in + (size_t)index * BLOCK_SECTOR_SIZE,
                   BLOCK_SECTOR_SIZE);
        __asm__ volatile ("mfence" ::: "memory");
        if (!issue_command(ATA_CMD_WRITE_DMA_EXT, lba + index, 1u, 1, 1)) {
            return 0;
        }
    }
    return 1;
}

static int ahci_flush_impl(struct block_device *device)
{
    (void)device;
    return issue_command(ATA_CMD_FLUSH_CACHE_EXT, 0u, 0u, 0, 0);
}

static int configure_port(struct hba_port *port)
{
    void *memory;

    if (!stop_port(port)) {
        return 0;
    }

    state.command_list_phys = pmm_alloc_page();
    state.fis_phys = pmm_alloc_page();
    state.command_table_phys = pmm_alloc_page();
    state.bounce_phys = pmm_alloc_page();
    if (state.command_list_phys == PADDR_INVALID ||
        state.fis_phys == PADDR_INVALID ||
        state.command_table_phys == PADDR_INVALID ||
        state.bounce_phys == PADDR_INVALID) {
        return 0;
    }

    memory = pmm_phys_to_hhdm(state.command_list_phys);
    if (memory == 0) return 0;
    clear_bytes(memory, VMM_PAGE_SIZE);
    state.command_list = (struct ahci_command_header *)memory;

    memory = pmm_phys_to_hhdm(state.fis_phys);
    if (memory == 0) return 0;
    clear_bytes(memory, VMM_PAGE_SIZE);

    memory = pmm_phys_to_hhdm(state.command_table_phys);
    if (memory == 0) return 0;
    clear_bytes(memory, VMM_PAGE_SIZE);
    state.command_table = (struct ahci_command_table *)memory;

    memory = pmm_phys_to_hhdm(state.bounce_phys);
    if (memory == 0) return 0;
    clear_bytes(memory, VMM_PAGE_SIZE);
    state.bounce = (uint8_t *)memory;

    port->clb = (uint32_t)state.command_list_phys;
    port->clbu = (uint32_t)(state.command_list_phys >> 32);
    port->fb = (uint32_t)state.fis_phys;
    port->fbu = (uint32_t)(state.fis_phys >> 32);
    port->ie = 0u;
    port->is = 0xFFFFFFFFu;
    port->serr = 0xFFFFFFFFu;

    start_port(port);
    return 1;
}

static uint64_t identify_capacity(void)
{
    const uint16_t *words;
    uint64_t sectors48;
    uint64_t sectors28;

    clear_bytes(state.bounce, BLOCK_SECTOR_SIZE);
    if (!issue_command(ATA_CMD_IDENTIFY, 0u, 1u, 0, 1)) {
        return 0u;
    }

    words = (const uint16_t *)(const void *)state.bounce;
    sectors48 = (uint64_t)words[100] |
        ((uint64_t)words[101] << 16) |
        ((uint64_t)words[102] << 32) |
        ((uint64_t)words[103] << 48);
    sectors28 = (uint64_t)words[60] | ((uint64_t)words[61] << 16);
    return sectors48 != 0u ? sectors48 : sectors28;
}

struct block_device *ahci_probe(void)
{
    struct pci_device controller;
    uint32_t bar5;
    paddr_t abar_phys;
    uint16_t command;
    uint32_t implemented;
    uint8_t port_number;

    if (initialized) {
        return state.block.sector_count != 0u ? &state.block : 0;
    }
    initialized = 1;

    if (!pci_find_class(
            PCI_CLASS_MASS_STORAGE,
            PCI_SUBCLASS_SATA,
            PCI_PROGIF_AHCI,
            &controller
        )) {
        return 0;
    }

    bar5 = pci_config_read32(
        controller.bus, controller.device, controller.function, PCI_BAR5
    );
    if ((bar5 & 1u) != 0u) {
        return 0;
    }
    abar_phys = (paddr_t)(bar5 & PCI_BAR_MEMORY_MASK);
    if (abar_phys == 0u || !map_ahci_mmio(abar_phys)) {
        return 0;
    }

    command = pci_config_read16(
        controller.bus, controller.device, controller.function, PCI_COMMAND
    );
    command |= PCI_COMMAND_MEMORY | PCI_COMMAND_BUS_MASTER;
    pci_config_write16(
        controller.bus, controller.device, controller.function, PCI_COMMAND,
        command
    );

    state.hba = (struct hba_memory *)(uintptr_t)AHCI_MMIO_VA;

    if ((state.hba->cap2 & HBA_CAP2_BOH) != 0u) {
        uint32_t remaining = AHCI_CMD_TIMEOUT;
        state.hba->bohc |= HBA_BOHC_OOS;
        while ((state.hba->bohc & (HBA_BOHC_BOS | HBA_BOHC_BB)) != 0u) {
            if (remaining-- == 0u) {
                return 0;
            }
            __asm__ volatile ("pause");
        }
    }

    state.hba->ghc |= HBA_GHC_AE;
    implemented = state.hba->pi;

    for (port_number = 0u; port_number < 32u; ++port_number) {
        struct hba_port *port;
        uint32_t ssts;

        if ((implemented & (1u << port_number)) == 0u) {
            continue;
        }

        port = &state.hba->ports[port_number];
        ssts = port->ssts;
        if ((ssts & HBA_SSTS_DET_MASK) != HBA_SSTS_DET_PRESENT ||
            (ssts & HBA_SSTS_IPM_MASK) != HBA_SSTS_IPM_ACTIVE ||
            port->sig != HBA_SIG_ATA) {
            continue;
        }

        state.port = port;
        state.port_number = port_number;
        if (!configure_port(port)) {
            return 0;
        }

        state.block.name = "ahci0";
        state.block.sector_size = BLOCK_SECTOR_SIZE;
        state.block.sector_count = identify_capacity();
        state.block.read = ahci_read_impl;
        state.block.write = ahci_write_impl;
        state.block.flush = ahci_flush_impl;
        state.block.private_data = &state;

        if (state.block.sector_count == 0u) {
            return 0;
        }
        return &state.block;
    }

    return 0;
}
