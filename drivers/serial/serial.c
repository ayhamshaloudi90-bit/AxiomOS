#include <stdint.h>

#include <axiom/arch/io.h>
#include <axiom/drivers/serial.h>


#define COM1_PORT 0x3F8u


static int serial_transmit_empty(void)
{
    return (inb(COM1_PORT + 5u) & 0x20u) != 0u;
}


void serial_init(void)
{
    /*
     * Disable UART interrupts.
     */
    outb(COM1_PORT + 1u, 0x00u);

    /*
     * Enable DLAB so ports 0 and 1 become the baud-rate
     * divisor registers.
     */
    outb(COM1_PORT + 3u, 0x80u);

    /*
 * Divisor = 1.
 *
 * 115200 / 1 = 115200 baud.
 */
outb(COM1_PORT + 0u, 0x01u);
outb(COM1_PORT + 1u, 0x00u);

    /*
     * Disable DLAB and configure:
     *
     * 8 data bits
     * 1 stop bit
     * no parity
     */
    outb(COM1_PORT + 3u, 0x03u);

    /*
     * Enable FIFO and clear transmit/receive queues.
     */
    outb(COM1_PORT + 2u, 0xC7u);

    /*
     * Enable DTR, RTS, and OUT2.
     */
    outb(COM1_PORT + 4u, 0x0Bu);
}


void serial_write_char(char character)
{
    /*
     * Terminals conventionally expect CRLF for a new line
     * over serial.
     */
    if (character == '\n') {
        serial_write_char('\r');
    }

    /*
     * Bit 5 of the Line Status Register means the transmitter
     * holding register is empty.
     */
    while (!serial_transmit_empty()) {
        /*
         * Wait until the UART can accept another character.
         */
    }

    outb(COM1_PORT, (uint8_t)character);
}


void serial_write_string(const char *string)
{
    while (*string != '\0') {
        serial_write_char(*string);
        ++string;
    }
}
