#ifndef AXIOM_SYNC_SELFTEST_H
#define AXIOM_SYNC_SELFTEST_H

#include <stdint.h>

enum sync_selftest_stage {
    SYNC_SELFTEST_STAGE_NONE = 0,
    SYNC_SELFTEST_STAGE_RACE = 1,
    SYNC_SELFTEST_STAGE_SPINLOCK = 2,
    SYNC_SELFTEST_STAGE_MUTEX = 3,
    SYNC_SELFTEST_STAGE_SEMAPHORE = 4
};

struct sync_selftest_result {
    uint64_t failure_stage;
    uint64_t race_expected;
    uint64_t race_actual;
    uint64_t spin_expected;
    uint64_t spin_actual;
    uint64_t spin_acquisitions;
    uint64_t mutex_expected;
    uint64_t mutex_actual;
    uint64_t mutex_blocks;
    uint64_t mutex_wakeups;
    uint64_t semaphore_limit;
    uint64_t semaphore_peak;
    uint64_t semaphore_blocks;
    uint64_t semaphore_wakeups;
};

int sync_run_selftest(struct sync_selftest_result *result);

#endif
