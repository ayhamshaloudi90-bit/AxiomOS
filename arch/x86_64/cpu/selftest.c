#include <stdint.h>
#include <axiom/arch/interrupts.h>
#include <axiom/terminal/kprintf.h>

extern uint64_t phase3_register_probe(void);
extern void phase3_trigger_divide(void);
extern void phase3_trigger_invalid(void);
extern void phase3_trigger_gp(void);

void phase3_selftest(void)
{
    /* Assembly checks all 15 saved GPRs, RSP and RFLAGS after IRETQ. */
    if (phase3_register_probe() != 1) {
        kprintf("Phase 3 register preservation: FAILED\n");
        __asm__ volatile ("ud2");
    }
    kprintf("Phase 3 register preservation: OK\n");
    __asm__ volatile ("int $0x80" ::: "memory");
    kprintf("Phase 3 CPU initialization complete.\n");
#if defined(AXIOM_TEST_DIVIDE)
    kprintf("Test: triggering real divide-by-zero.\n");
    phase3_trigger_divide();
#elif defined(AXIOM_TEST_INVALID)
    kprintf("Test: triggering UD2.\n");
    phase3_trigger_invalid();
#elif defined(AXIOM_TEST_GP)
    kprintf("Test: triggering invalid segment selector.\n");
    phase3_trigger_gp();
#elif defined(AXIOM_TEST_DOUBLE_FAULT)
    extern void phase3_disable_gp_gate(void);
    kprintf("Test: triggering double fault.\n");
    phase3_disable_gp_gate();
    phase3_trigger_gp();
#endif
}
