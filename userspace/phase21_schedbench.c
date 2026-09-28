#include <stdint.h>
#include <stdio.h>
#include <axiom/abi/scheduler.h>
#include <axiom/syscalls.h>

#define BENCH_TASKS 3u
#define BENCH_DECISIONS 96u
#define BENCH_QUANTUM_TICKS 5u
#define BENCH_AGING_TICKS 20u

static void simulate_round_robin(uint64_t counts[BENCH_TASKS])
{
    uint64_t decision;

    for (decision = 0u; decision < BENCH_DECISIONS; ++decision) {
        ++counts[decision % BENCH_TASKS];
    }
}

static void simulate_priority_aging(uint64_t counts[BENCH_TASKS])
{
    static const uint32_t base_priority[BENCH_TASKS] = {1u, 4u, 7u};
    uint64_t waiting_ticks[BENCH_TASKS] = {0u, 0u, 0u};
    unsigned previous = BENCH_TASKS - 1u;
    uint64_t decision;

    for (decision = 0u; decision < BENCH_DECISIONS; ++decision) {
        unsigned selected = BENCH_TASKS;
        uint32_t selected_priority = 0u;
        unsigned step;
        unsigned task;

        for (step = 1u; step <= BENCH_TASKS; ++step) {
            const unsigned index = (previous + step) % BENCH_TASKS;
            uint32_t effective = base_priority[index] +
                (uint32_t)(waiting_ticks[index] / BENCH_AGING_TICKS);

            if (effective > AXIOM_SCHED_PRIORITY_MAX) {
                effective = AXIOM_SCHED_PRIORITY_MAX;
            }
            if (selected == BENCH_TASKS || effective > selected_priority) {
                selected = index;
                selected_priority = effective;
            }
        }

        ++counts[selected];
        for (task = 0u; task < BENCH_TASKS; ++task) {
            if (task == selected) {
                waiting_ticks[task] = 0u;
            } else {
                waiting_ticks[task] += BENCH_QUANTUM_TICKS;
            }
        }
        previous = selected;
    }
}

int main(void)
{
    uint64_t rr[BENCH_TASKS] = {0u, 0u, 0u};
    uint64_t advanced[BENCH_TASKS] = {0u, 0u, 0u};
    struct axiom_scheduler_stats stats;
    long original_priority;
    long result;

    puts("Phase 21 scheduler benchmark");

    simulate_round_robin(rr);
    simulate_priority_aging(advanced);

    printf("Round-robin selections: %llu/%llu/%llu\n",
        (unsigned long long)rr[0],
        (unsigned long long)rr[1],
        (unsigned long long)rr[2]);
    printf("Priority+aging selections: %llu/%llu/%llu\n",
        (unsigned long long)advanced[0],
        (unsigned long long)advanced[1],
        (unsigned long long)advanced[2]);

    result = axiom_schedstats(&stats);
    if (result < 0 || stats.policy != AXIOM_SCHED_POLICY_PRIORITY_AGING) {
        puts("Phase 21 scheduler stats API: FAILED");
        return 1;
    }
    printf("Live scheduler policy: priority-aging\n");
    printf("Live switches/preemptions: %llu/%llu\n",
        (unsigned long long)stats.context_switches,
        (unsigned long long)stats.preemptions);
    printf("Live priority preemptions/aging promotions: %llu/%llu\n",
        (unsigned long long)stats.priority_preemptions,
        (unsigned long long)stats.aging_promotions);

    original_priority = axiom_getpriority();
    if (original_priority < 0 ||
        axiom_setpriority(AXIOM_SCHED_PRIORITY_MAX) < 0 ||
        axiom_getpriority() != AXIOM_SCHED_PRIORITY_MAX ||
        axiom_setpriority((unsigned long)original_priority) < 0) {
        puts("Phase 21 priority API: FAILED");
        return 2;
    }
    puts("Phase 21 priority API: OK");

    if (axiom_setaffinity(AXIOM_CPU_AFFINITY_BSP) < 0 ||
        axiom_getaffinity() != (long)AXIOM_CPU_AFFINITY_BSP) {
        puts("Phase 21 affinity API: FAILED");
        return 3;
    }
    puts("Phase 21 affinity API: OK");

    if (rr[0] != rr[1] || rr[1] != rr[2] ||
        advanced[0] == 0u || advanced[1] == 0u || advanced[2] == 0u ||
        advanced[2] <= advanced[0]) {
        puts("Phase 21 benchmark comparison: FAILED");
        return 4;
    }
    puts("Phase 21 benchmark comparison: OK");
    puts("Phase 21 scheduler benchmark complete.");
    return 0;
}
