#include <stdint.h>

#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>
#include <axiom/terminal/kprintf.h>


static int selftest_fail(const char *reason)
{
    kprintf("Phase 5 virtual memory test: FAILED (%s)\n", reason);
    return 0;
}


int phase5_vmm_selftest(void)
{
    const uint64_t first_signature = 0xA5105A105A10C0DEULL;
    const uint64_t second_signature = 0x5A01A5010BADC0DEULL;

    struct pmm_stats pmm_before;
    struct pmm_stats pmm_after;
    struct vmm_stats vmm_before;
    struct vmm_stats vmm_after;

    paddr_t page;
    paddr_t translated;

    volatile uint64_t *virtual_alias;
    volatile uint64_t *hhdm_alias;


    if (virt_to_phys(VMM_SELFTEST_ADDRESS) != PADDR_INVALID) {
        return selftest_fail("test virtual address was already mapped");
    }


    pmm_before = pmm_get_stats();
    vmm_before = vmm_get_stats();


    page = pmm_alloc_page();

    if (page == PADDR_INVALID) {
        return selftest_fail("physical allocation failed");
    }


    if (!map_page(
            VMM_SELFTEST_ADDRESS,
            page,
            VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE)) {

        (void)pmm_free_page(page);
        return selftest_fail("map_page failed");
    }


    translated = virt_to_phys(VMM_SELFTEST_ADDRESS);

    if (translated != page) {
        (void)unmap_page(VMM_SELFTEST_ADDRESS);
        (void)pmm_free_page(page);
        return selftest_fail("virt_to_phys returned wrong page");
    }


    virtual_alias =
        (volatile uint64_t *)(uintptr_t)VMM_SELFTEST_ADDRESS;

    hhdm_alias =
        (volatile uint64_t *)pmm_phys_to_hhdm(page);


    if (hhdm_alias == 0) {
        (void)unmap_page(VMM_SELFTEST_ADDRESS);
        (void)pmm_free_page(page);
        return selftest_fail("HHDM alias unavailable");
    }


    *virtual_alias = first_signature;

    if (*hhdm_alias != first_signature) {
        (void)unmap_page(VMM_SELFTEST_ADDRESS);
        (void)pmm_free_page(page);
        return selftest_fail("virtual write did not reach physical page");
    }


    *hhdm_alias = second_signature;

    if (*virtual_alias != second_signature) {
        (void)unmap_page(VMM_SELFTEST_ADDRESS);
        (void)pmm_free_page(page);
        return selftest_fail("HHDM write not visible through mapping");
    }


    if (map_page(
            VMM_SELFTEST_ADDRESS,
            page,
            VMM_FLAG_WRITABLE)) {

        (void)unmap_page(VMM_SELFTEST_ADDRESS);
        (void)pmm_free_page(page);
        return selftest_fail("duplicate mapping was accepted");
    }


    if (!unmap_page(VMM_SELFTEST_ADDRESS)) {
        (void)pmm_free_page(page);
        return selftest_fail("unmap_page failed");
    }


    if (virt_to_phys(VMM_SELFTEST_ADDRESS) != PADDR_INVALID) {
        (void)pmm_free_page(page);
        return selftest_fail("translation survived unmap");
    }


    if (unmap_page(VMM_SELFTEST_ADDRESS)) {
        (void)pmm_free_page(page);
        return selftest_fail("second unmap was accepted");
    }


    if (!pmm_free_page(page)) {
        return selftest_fail("physical page free failed");
    }


    pmm_after = pmm_get_stats();
    vmm_after = vmm_get_stats();


    if (pmm_after.allocated_pages != pmm_before.allocated_pages ||
        pmm_after.free_pages != pmm_before.free_pages) {

        return selftest_fail("PMM accounting was not restored");
    }


    if (vmm_after.page_table_pages != vmm_before.page_table_pages) {
        return selftest_fail("temporary page-table pages leaked");
    }


    kprintf("Phase 5 virtual memory test: OK\n");
    return 1;
}
