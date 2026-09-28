#include <stdint.h>

#include <axiom/memory/pmm.h>
#include <axiom/terminal/kprintf.h>


static int selftest_fail(const char *reason)
{
    kprintf("Phase 4 PMM allocation/free test: FAILED (%s)\n", reason);
    return 0;
}


int phase4_pmm_selftest(void)
{
    struct pmm_stats before;
    struct pmm_stats during;
    struct pmm_stats after;

    paddr_t first;
    paddr_t second;
    paddr_t third;

    volatile uint64_t *first_page;
    volatile uint64_t *second_page;
    volatile uint64_t *third_page;

    const uint64_t words_per_page = PMM_PAGE_SIZE / sizeof(uint64_t);


    before = pmm_get_stats();


    first = pmm_alloc_page();
    second = pmm_alloc_page();
    third = pmm_alloc_page();


    if (first == PADDR_INVALID ||
        second == PADDR_INVALID ||
        third == PADDR_INVALID) {

        return selftest_fail("allocation returned no page");
    }


    if ((first & (PMM_PAGE_SIZE - 1ULL)) != 0 ||
        (second & (PMM_PAGE_SIZE - 1ULL)) != 0 ||
        (third & (PMM_PAGE_SIZE - 1ULL)) != 0) {

        return selftest_fail("unaligned physical page");
    }


    if (first == second || first == third || second == third) {
        return selftest_fail("duplicate allocation");
    }


    during = pmm_get_stats();

    if (during.allocated_pages != before.allocated_pages + 3ULL ||
        during.free_pages + 3ULL != before.free_pages) {

        return selftest_fail("accounting after allocation");
    }


    first_page = (volatile uint64_t *)pmm_phys_to_hhdm(first);
    second_page = (volatile uint64_t *)pmm_phys_to_hhdm(second);
    third_page = (volatile uint64_t *)pmm_phys_to_hhdm(third);


    if (first_page == 0 || second_page == 0 || third_page == 0) {
        return selftest_fail("HHDM conversion");
    }


    first_page[0] = 0xA110C001A110C001ULL;
    first_page[words_per_page - 1ULL] = 0xA110C001FFFFFFFFULL;

    second_page[0] = 0xB220C002B220C002ULL;
    second_page[words_per_page - 1ULL] = 0xB220C002FFFFFFFFULL;

    third_page[0] = 0xC330C003C330C003ULL;
    third_page[words_per_page - 1ULL] = 0xC330C003FFFFFFFFULL;


    if (first_page[0] != 0xA110C001A110C001ULL ||
        first_page[words_per_page - 1ULL] != 0xA110C001FFFFFFFFULL ||
        second_page[0] != 0xB220C002B220C002ULL ||
        second_page[words_per_page - 1ULL] != 0xB220C002FFFFFFFFULL ||
        third_page[0] != 0xC330C003C330C003ULL ||
        third_page[words_per_page - 1ULL] != 0xC330C003FFFFFFFFULL) {

        return selftest_fail("page read/write verification");
    }


    if (pmm_free_page(first + 1ULL)) {
        return selftest_fail("accepted unaligned free");
    }


    if (!pmm_free_page(second) ||
        !pmm_free_page(first) ||
        !pmm_free_page(third)) {

        return selftest_fail("valid free rejected");
    }


    if (pmm_free_page(third)) {
        return selftest_fail("double free accepted");
    }


    after = pmm_get_stats();

    if (after.allocated_pages != before.allocated_pages ||
        after.free_pages != before.free_pages) {

        return selftest_fail("accounting after free");
    }


    kprintf("Phase 4 PMM allocation/free test: OK\n");
    return 1;
}
