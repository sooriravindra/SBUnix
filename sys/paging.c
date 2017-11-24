#include <sys/alloc.h>
#include <sys/kprintf.h>
#include <sys/paging.h>
#include <test.h>

#define PAGING_PAGETABLE_PERMISSIONS 0x7
#define PAGING_PAGELIST_ENTRIES (1024 * 1024)
#define PAGING_TABLE_ENTRIES 512
#define TEST_PAGING (PAGING_KERNMEM + 0x801000)

// TODO Allocate pages array dynamically
struct pagelist_t pages[PAGING_PAGELIST_ENTRIES];
struct pagelist_t* freepage_head;
extern void paging_enable(void*);

void
paging_pagelist_add_addresses(uint64_t start, uint64_t end)
{
    // Call with start and end physical addresses that you want to
    // be bookkept in the FREELIST
    for (int i = start / PAGING_PAGE_SIZE; i < end / PAGING_PAGE_SIZE; i++) {
        pages[i].present = TRUE;
    }
}

void
paging_pagelist_create(uint64_t physfree)
{
    // Once you add all addresses to be bookkept in the FREELIST by
    // calling paging_pagelist_add_addresses, call paging_pagelist_create
    // The parameter signifies the page boundary from where free
    // pages are assigned

    int i = physfree / PAGING_PAGE_SIZE;
    bool isLastPage = FALSE;
    while (!isLastPage) {
        isLastPage = TRUE;
        for (int j = i + 1; j < PAGING_PAGELIST_ENTRIES; j++) {
            if (pages[j].present) {
                pages[i].next = &pages[j];
                isLastPage = FALSE;
                i = j;
                break;
            }
        }
    }
    freepage_head = &pages[(physfree / PAGING_PAGE_SIZE)];
}

void*
paging_pagelist_get_frame()
{
    // Returns a freepage from the FREELIST
    uint64_t pageAddress = (freepage_head - pages) * PAGING_PAGE_SIZE;
    if (freepage_head == NULL) {
        return NULL;
        kprintf("Out of memory");
        while (1)
            ;
    }
    freepage_head = freepage_head->next;
    return (void*)pageAddress;
}

void
paging_pagelist_free_frame()
{
    // TODO Free frame
}

uint64_t*
paging_get_table_entry(uint64_t* table, uint32_t offset)
{
    // Adds entry into offset of the table if not present, returns entry
    /*char* temp_byte;*/
    if (!(table[offset] & 0x1)) {
        table[offset] = (uint64_t)paging_pagelist_get_frame();

        /*
        // TODO Decide if it is needed to set page to 0
        temp_byte = (char*)table[offset];
        for (int i = 0; i < PAGING_PAGE_SIZE; i++) {
            temp_byte[i] = 0;
        }
        */
        table[offset] |= PAGING_PAGETABLE_PERMISSIONS;
    }
    return (uint64_t*)(table[offset] & 0xfffffffffffff000);
}

bool
paging_add_pagetable_mapping(uint64_t v_addr, uint64_t p_addr)
{
    // The pagetables' addresses can be calculated because of self referencing
    // trick
    uint64_t pml4_vaddr, pdp_vaddr, pd_vaddr, pt_vaddr;
    uint64_t* pagetable;
    uint32_t pt_offset;

    pml4_vaddr = (~0 << 12);
    paging_get_table_entry((uint64_t*)pml4_vaddr, PAGING_PML4_OFFSET(v_addr));

    pdp_vaddr = ((pml4_vaddr << 9) | (PAGING_PML4_OFFSET(v_addr) << 12));
    paging_get_table_entry((uint64_t*)pdp_vaddr,
                           PAGING_PD_POINTER_OFFSET(v_addr));

    pd_vaddr = ((pdp_vaddr << 9) | (PAGING_PD_POINTER_OFFSET(v_addr) << 12));
    paging_get_table_entry((uint64_t*)pd_vaddr,
                           PAGING_PAGE_DIRECTORY_OFFSET(v_addr));

    pt_vaddr = ((pd_vaddr << 9) | (PAGING_PAGE_DIRECTORY_OFFSET(v_addr) << 12));
    pt_offset = PAGING_PAGE_TABLE_OFFSET(v_addr);

    pagetable = (uint64_t*)pt_vaddr;

    if ((pagetable[pt_offset] & 0x1)) {
        return FALSE;
    } else {
        pagetable[pt_offset] = p_addr;
        /*
        // TODO Decide if it is needed to set page to 0
        char* temp_byte = (char*)pagetable[pt_offset];
        for (int i = 0; i < PAGE_SIZE; i++) {
            temp_byte[i] = 0;
        }
        */
        pagetable[pt_offset] |= PAGING_PAGETABLE_PERMISSIONS;
        return TRUE;
    }
}

void
paging_add_initial_pagetable_mapping(uint64_t* pml4_phys_addr, uint64_t v_addr,
                                     uint64_t phys_entry)
{
    // This function can add mapping if the physcial <-> virtual mapping is 1:1
    // Thus can be used to setup initial pagetables only
    uint64_t* pdp_table;
    uint64_t* pd_table;
    uint64_t* pt_table;

    pdp_table =
      paging_get_table_entry(pml4_phys_addr, PAGING_PML4_OFFSET(v_addr));

    pd_table =
      paging_get_table_entry(pdp_table, PAGING_PD_POINTER_OFFSET(v_addr));

    pt_table =
      paging_get_table_entry(pd_table, PAGING_PAGE_DIRECTORY_OFFSET(v_addr));

    pt_table[PAGING_PAGE_TABLE_OFFSET(v_addr)] = phys_entry;
}

void
paging_create_pagetables()
{
    // Creates the 4 level pagetables needed and switches CR3
    uint64_t* pml4_table = paging_pagelist_get_frame();
    uint64_t v_addr;

    for (int i = 0; i < PAGING_TABLE_ENTRIES; i++) {
        pml4_table[i] = 0;
    }

    // Self referencing trick
    pml4_table[PAGING_TABLE_ENTRIES - 1] =
      ((uint64_t)pml4_table) | PAGING_PAGETABLE_PERMISSIONS;

    // TODO Remove this hard coding of 2048. Map only physbase to physfree
    for (int i = 0; i < 5000; i++) {
        v_addr = PAGING_KERNMEM + i * (PAGING_PAGE_SIZE);
        paging_add_initial_pagetable_mapping(pml4_table, v_addr,
                                             (i * PAGING_PAGE_SIZE) |
                                               PAGING_PAGETABLE_PERMISSIONS);
    }

    paging_add_initial_pagetable_mapping(
      pml4_table, PAGING_VIDEO, (0xb8000) | PAGING_PAGETABLE_PERMISSIONS);
    /*paging_add_initial_pagetable_mapping(pml4_table, TEST_PAGING,
     * (test_address) |
     * PAGING_PAGETABLE_PERMISSIONS);*/

    paging_enable(pml4_table);

    // test_get_free_pages();
}