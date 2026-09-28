OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)


PHDRS
{
    requests PT_LOAD FLAGS(6);
    text     PT_LOAD FLAGS(5);
    rodata   PT_LOAD FLAGS(4);
    data     PT_LOAD FLAGS(6);
}


SECTIONS
{
    /*
     * AxiomOS is a higher-half kernel.
     */
    . = 0xffffffff80000000;


    /*
     * Limine protocol structures.
     */
    .limine_requests : ALIGN(8)
    {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :requests


    /*
     * Executable kernel code.
     */
    . = ALIGN(0x1000);

    .text :
    {
        *(.text .text.*)
    } :text


    /*
     * Read-only data such as string literals.
     */
    . = ALIGN(0x1000);

    .rodata :
    {
        *(.rodata .rodata.*)
    } :rodata


    /*
     * Writable initialized data.
     */
    . = ALIGN(0x1000);

    .data :
    {
        *(.data .data.*)
    } :data


    /*
     * Zero-initialized data.
     *
     * Our bootstrap stack lives here.
     */
    .bss (NOLOAD) :
    {
        *(COMMON)
        *(.bss .bss.*)
    } :data


    /*
     * These are unnecessary for our freestanding kernel.
     */
    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.note*)
    }
}
