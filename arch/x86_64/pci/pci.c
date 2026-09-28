#include <stdint.h>

#include <axiom/arch/io.h>
#include <axiom/arch/pci.h>

#define PCI_CONFIG_ADDRESS 0xCF8u
#define PCI_CONFIG_DATA    0xCFCu
#define PCI_ENABLE         0x80000000u

static uint32_t pci_address(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
)
{
    return PCI_ENABLE |
        ((uint32_t)bus << 16) |
        ((uint32_t)(device & 0x1Fu) << 11) |
        ((uint32_t)(function & 0x07u) << 8) |
        ((uint32_t)offset & 0xFCu);
}

uint32_t pci_config_read32(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
)
{
    outl(PCI_CONFIG_ADDRESS, pci_address(bus, device, function, offset));
    return inl(PCI_CONFIG_DATA);
}

void pci_config_write32(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset,
    uint32_t value
)
{
    outl(PCI_CONFIG_ADDRESS, pci_address(bus, device, function, offset));
    outl(PCI_CONFIG_DATA, value);
}

uint16_t pci_config_read16(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
)
{
    outl(PCI_CONFIG_ADDRESS, pci_address(bus, device, function, offset));
    return inw((uint16_t)(PCI_CONFIG_DATA + (offset & 2u)));
}

void pci_config_write16(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset,
    uint16_t value
)
{
    outl(PCI_CONFIG_ADDRESS, pci_address(bus, device, function, offset));
    outw((uint16_t)(PCI_CONFIG_DATA + (offset & 2u)), value);
}

int pci_find_device(
    uint16_t vendor_id,
    uint16_t device_id,
    struct pci_device *device_out
)
{
    uint16_t bus;

    if (device_out == 0) return 0;

    for (bus = 0u; bus < 256u; ++bus) {
        uint8_t device;
        for (device = 0u; device < 32u; ++device) {
            uint8_t function;
            uint8_t max_functions = 1u;

            for (function = 0u; function < max_functions; ++function) {
                const uint32_t id = pci_config_read32(
                    (uint8_t)bus, device, function, 0x00u
                );
                uint32_t class_reg;
                uint8_t header_type;

                if ((id & 0xFFFFu) == 0xFFFFu) continue;

                if (function == 0u) {
                    header_type = (uint8_t)(
                        pci_config_read32((uint8_t)bus, device, 0u, 0x0Cu) >> 16
                    );
                    if ((header_type & 0x80u) != 0u) max_functions = 8u;
                }

                if ((uint16_t)(id & 0xFFFFu) != vendor_id ||
                    (uint16_t)(id >> 16) != device_id) {
                    continue;
                }

                class_reg = pci_config_read32(
                    (uint8_t)bus, device, function, 0x08u
                );
                device_out->bus = (uint8_t)bus;
                device_out->device = device;
                device_out->function = function;
                device_out->vendor_id = vendor_id;
                device_out->device_id = device_id;
                device_out->class_code = (uint8_t)(class_reg >> 24);
                device_out->subclass = (uint8_t)(class_reg >> 16);
                device_out->prog_if = (uint8_t)(class_reg >> 8);
                return 1;
            }
        }
    }
    return 0;
}

int pci_find_class(
    uint8_t class_code,
    uint8_t subclass,
    uint8_t prog_if,
    struct pci_device *device_out
)
{
    uint16_t bus;

    if (device_out == 0) {
        return 0;
    }

    for (bus = 0u; bus < 256u; ++bus) {
        uint8_t device;

        for (device = 0u; device < 32u; ++device) {
            uint8_t function;
            uint8_t max_functions = 1u;

            for (function = 0u; function < max_functions; ++function) {
                const uint32_t id = pci_config_read32(
                    (uint8_t)bus, device, function, 0x00u
                );
                uint32_t class_reg;
                uint8_t header_type;

                if ((id & 0xFFFFu) == 0xFFFFu) {
                    continue;
                }

                if (function == 0u) {
                    header_type = (uint8_t)(
                        pci_config_read32((uint8_t)bus, device, 0u, 0x0Cu) >> 16
                    );
                    if ((header_type & 0x80u) != 0u) {
                        max_functions = 8u;
                    }
                }

                class_reg = pci_config_read32(
                    (uint8_t)bus, device, function, 0x08u
                );

                if ((uint8_t)(class_reg >> 24) == class_code &&
                    (uint8_t)(class_reg >> 16) == subclass &&
                    (uint8_t)(class_reg >> 8) == prog_if) {
                    device_out->bus = (uint8_t)bus;
                    device_out->device = device;
                    device_out->function = function;
                    device_out->vendor_id = (uint16_t)(id & 0xFFFFu);
                    device_out->device_id = (uint16_t)(id >> 16);
                    device_out->class_code = class_code;
                    device_out->subclass = subclass;
                    device_out->prog_if = prog_if;
                    return 1;
                }
            }
        }
    }

    return 0;
}
