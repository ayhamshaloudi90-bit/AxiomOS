#ifndef AXIOM_DRIVERS_SERIAL_H
#define AXIOM_DRIVERS_SERIAL_H


void serial_init(void);

void serial_write_char(char character);

void serial_write_string(const char *string);


#endif
