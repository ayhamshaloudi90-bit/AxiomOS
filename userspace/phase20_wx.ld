ENTRY(_start)
PHDRS
{
    bad PT_LOAD FLAGS(7);
}
SECTIONS
{
    . = 0x0000000000400000;
    .phase20wx : ALIGN(0x1000) { *(.phase20wx*) } :bad
    /DISCARD/ : { *(.comment*) *(.eh_frame*) }
}
