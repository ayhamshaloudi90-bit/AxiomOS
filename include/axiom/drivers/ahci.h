#ifndef AXIOM_DRIVERS_AHCI_H
#define AXIOM_DRIVERS_AHCI_H

#include <axiom/drivers/block.h>

struct block_device *ahci_probe(void);

#endif
