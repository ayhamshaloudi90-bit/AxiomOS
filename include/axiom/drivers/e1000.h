#ifndef AXIOM_DRIVERS_E1000_H
#define AXIOM_DRIVERS_E1000_H

#include <stddef.h>
#include <stdint.h>

#define E1000_ETHERNET_FRAME_MAX 1518u

struct e1000_stats {
    uint64_t tx_frames;
    uint64_t rx_frames;
    uint64_t tx_bytes;
    uint64_t rx_bytes;
    uint64_t tx_errors;
    uint64_t rx_dropped;
};

/* Probe and initialise the QEMU/Intel 82540EM E1000 PCI NIC. */
int e1000_init(void);
int e1000_available(void);

/* The Phase-18 driver is deliberately polled; interrupts come later. */
int e1000_send(const void *frame, size_t length);
int e1000_receive(void *frame, size_t capacity, size_t *length_out);

void e1000_mac(uint8_t mac_out[6]);
struct e1000_stats e1000_get_stats(void);

#endif
