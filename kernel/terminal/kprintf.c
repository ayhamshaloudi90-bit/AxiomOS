#include <stdarg.h>
#include <stdint.h>

#include <axiom/terminal/kprintf.h>
#include <axiom/terminal/terminal.h>


enum length_modifier {
    LENGTH_DEFAULT,

    LENGTH_LONG,

    LENGTH_LONG_LONG
};


static void print_unsigned(
    uint64_t value,
    uint32_t base,
    int uppercase
)
{
    char buffer[65];

    const char *digits;

    uint32_t index;


    digits =
        uppercase
            ? "0123456789ABCDEF"
            : "0123456789abcdef";


    if (value == 0u) {
        terminal_putchar('0');

        return;
    }


    index = 0;


    while (value != 0u) {
        buffer[index++] =
            digits[value % base];

        value /= base;
    }


    /*
     * Digits were produced backwards.
     */
    while (index > 0u) {
        terminal_putchar(
            buffer[--index]
        );
    }
}


static void print_signed(int64_t value)
{
    uint64_t magnitude;


    if (value < 0) {
        terminal_putchar('-');


        /*
         * This form safely handles INT64_MIN.
         */
        magnitude =
            (uint64_t)(-(value + 1))
            +
            1u;
    } else {
        magnitude =
            (uint64_t)value;
    }


    print_unsigned(
        magnitude,
        10u,
        0
    );
}


static int64_t read_signed_argument(
    va_list *arguments,
    enum length_modifier length
)
{
    switch (length) {
        case LENGTH_LONG:

            return (int64_t)va_arg(
                *arguments,
                long
            );


        case LENGTH_LONG_LONG:

            return (int64_t)va_arg(
                *arguments,
                long long
            );


        case LENGTH_DEFAULT:
        default:

            return (int64_t)va_arg(
                *arguments,
                int
            );
    }
}


static uint64_t read_unsigned_argument(
    va_list *arguments,
    enum length_modifier length
)
{
    switch (length) {
        case LENGTH_LONG:

            return (uint64_t)va_arg(
                *arguments,
                unsigned long
            );


        case LENGTH_LONG_LONG:

            return (uint64_t)va_arg(
                *arguments,
                unsigned long long
            );


        case LENGTH_DEFAULT:
        default:

            return (uint64_t)va_arg(
                *arguments,
                unsigned int
            );
    }
}


void kprintf(const char *format, ...)
{
    va_list arguments;


    if (format == 0) {
        return;
    }


    va_start(
        arguments,
        format
    );


    while (*format != '\0') {
        enum length_modifier length;


        if (*format != '%') {
            terminal_putchar(
                *format++
            );

            continue;
        }


        ++format;


        /*
         * %%
         */
        if (*format == '%') {
            terminal_putchar('%');

            ++format;

            continue;
        }


        length =
            LENGTH_DEFAULT;


        /*
         * %l...
         * %ll...
         */
        if (*format == 'l') {
            length =
                LENGTH_LONG;

            ++format;


            if (*format == 'l') {
                length =
                    LENGTH_LONG_LONG;

                ++format;
            }
        }


        switch (*format) {
            /*
             * Character
             */
            case 'c':

                terminal_putchar(
                    (char)va_arg(
                        arguments,
                        int
                    )
                );

                break;


            /*
             * String
             */
            case 's':

                terminal_write(
                    va_arg(
                        arguments,
                        const char *
                    )
                );

                break;


            /*
             * Signed decimal
             */
            case 'd':
            case 'i':

                print_signed(
                    read_signed_argument(
                        &arguments,
                        length
                    )
                );

                break;


            /*
             * Unsigned decimal
             */
            case 'u':

                print_unsigned(
                    read_unsigned_argument(
                        &arguments,
                        length
                    ),

                    10u,

                    0
                );

                break;


            /*
             * Lowercase hexadecimal
             */
            case 'x':

                print_unsigned(
                    read_unsigned_argument(
                        &arguments,
                        length
                    ),

                    16u,

                    0
                );

                break;


            /*
             * Uppercase hexadecimal
             */
            case 'X':

                print_unsigned(
                    read_unsigned_argument(
                        &arguments,
                        length
                    ),

                    16u,

                    1
                );

                break;


            /*
             * Pointer
             */
            case 'p':

                terminal_write("0x");


                print_unsigned(
                    (uint64_t)(uintptr_t)
                        va_arg(
                            arguments,
                            void *
                        ),

                    16u,

                    0
                );

                break;


            /*
             * Format ended with %.
             */
            case '\0':

                terminal_putchar('%');

                va_end(arguments);

                return;


            /*
             * Unknown format.
             *
             * Print it literally rather than silently
             * swallowing the user's output.
             */
            default:

                terminal_putchar('%');


                if (length == LENGTH_LONG) {
                    terminal_putchar('l');
                } else if (
                    length ==
                    LENGTH_LONG_LONG
                ) {
                    terminal_write("ll");
                }


                terminal_putchar(*format);

                break;
        }


        ++format;
    }


    va_end(arguments);
}
