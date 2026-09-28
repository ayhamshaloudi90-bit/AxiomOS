#include <axiom/arch/io.h>
#include <axiom/arch/pic.h>

#define PIC1_COMMAND 0x20u
#define PIC1_DATA    0x21u
#define PIC2_COMMAND 0xA0u
#define PIC2_DATA    0xA1u

static void pic_write(uint16_t port, uint8_t value)
{
    outb(port, value);
    outb(0x80u, 0u); /* Traditional I/O delay for the legacy controller. */
}

void pic_init_masked(void)
{
    pic_write(PIC1_DATA, 0xFFu);
    pic_write(PIC2_DATA, 0xFFu);
    pic_write(PIC1_COMMAND, 0x11u); /* ICW1: initialize, ICW4 follows. */
    pic_write(PIC2_COMMAND, 0x11u);
    pic_write(PIC1_DATA, 0x20u);    /* ICW2: master vectors 32..39. */
    pic_write(PIC2_DATA, 0x28u);    /* Slave vectors 40..47. */
    pic_write(PIC1_DATA, 0x04u);    /* ICW3: slave wired to master IRQ2. */
    pic_write(PIC2_DATA, 0x02u);
    pic_write(PIC1_DATA, 0x01u);    /* ICW4: 8086 mode, explicit EOI. */
    pic_write(PIC2_DATA, 0x01u);
    pic_write(PIC1_DATA, 0xFFu);    /* Phase 3 starts with every IRQ masked. */
    pic_write(PIC2_DATA, 0xFFu);
}

int pic_mask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t bit;
    uint8_t mask;

    if (irq >= 16u) {
        return 0;
    }

    if (irq < 8u) {
        port = PIC1_DATA;
        bit = irq;
    } else {
        port = PIC2_DATA;
        bit = (uint8_t)(irq - 8u);
    }

    mask = inb(port);
    mask |= (uint8_t)(1u << bit);
    pic_write(port, mask);
    return 1;
}

int pic_unmask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t bit;
    uint8_t mask;

    if (irq >= 16u) {
        return 0;
    }

    if (irq < 8u) {
        port = PIC1_DATA;
        bit = irq;
    } else {
        /* Any slave IRQ also needs the master's IRQ2 cascade unmasked. */
        mask = inb(PIC1_DATA);
        mask &= (uint8_t)~(1u << 2);
        pic_write(PIC1_DATA, mask);

        port = PIC2_DATA;
        bit = (uint8_t)(irq - 8u);
    }

    mask = inb(port);
    mask &= (uint8_t)~(1u << bit);
    pic_write(port, mask);
    return 1;
}

int pic_is_spurious(uint8_t irq)
{
    if (irq == 7u) {
        outb(PIC1_COMMAND, 0x0Bu); /* OCW3: read in-service register. */
        return (inb(PIC1_COMMAND) & 0x80u) == 0u;
    }

    if (irq == 15u) {
        outb(PIC2_COMMAND, 0x0Bu);

        if ((inb(PIC2_COMMAND) & 0x80u) == 0u) {
            /* Slave did not accept IRQ15, but master serviced cascade. */
            outb(PIC1_COMMAND, 0x20u);
            return 1;
        }
    }

    return 0;
}

void pic_eoi(uint8_t irq)
{
    if (irq >= 8u) {
        outb(PIC2_COMMAND, 0x20u);
    }

    outb(PIC1_COMMAND, 0x20u);
}
