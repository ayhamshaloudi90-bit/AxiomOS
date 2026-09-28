#include <stdint.h>

#include <axiom/arch/apic.h>
#include <axiom/arch/interrupts.h>
#include <axiom/drivers/timer.h>
#include <axiom/process/scheduler.h>

#define TIMER_IRQ    0u
#define TIMER_VECTOR 32u

static volatile uint64_t tick_count;
static uint32_t configured_frequency;

static struct interrupt_frame *timer_irq_handler(struct interrupt_frame *frame)
{
    ++tick_count;
    return scheduler_on_timer_interrupt(frame);
}

int timer_init(uint32_t frequency_hz)
{
    if (frequency_hz == 0u || interrupts_enabled()) {
        return 0;
    }

    if (!apic_init()) {
        return 0;
    }

    if (irq_register(TIMER_IRQ, timer_irq_handler) != 0) {
        return 0;
    }

    tick_count = 0u;

    if (!apic_timer_start(TIMER_VECTOR, frequency_hz)) {
        return 0;
    }

    configured_frequency = apic_timer_frequency();
    return configured_frequency != 0u;
}

uint64_t timer_ticks(void)
{
    return tick_count;
}

uint32_t timer_frequency(void)
{
    return configured_frequency;
}

void timer_wait_ticks(uint64_t count)
{
    const uint64_t start = timer_ticks();

    while ((timer_ticks() - start) < count) {
        __asm__ volatile ("hlt" ::: "memory");
    }
}
