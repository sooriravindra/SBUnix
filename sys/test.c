#include <string.h>
#include <sys/alloc.h>
#include <sys/elf64.h>
#include <sys/kprintf.h>
#include <sys/paging.h>
#include <sys/tarfs.h>
#include <sys/task.h>
#include <sys/tasklist.h>
#include <sys/term.h>
#include <sys/timer.h>
#include <sys/vma.h>
#include <test.h>
uint64_t test_address;

struct test_struct
{
    int num;
    char* str;
};

void
test_alloc_get_page()
{
    uint64_t* p = (uint64_t*)alloc_get_page();
    *p = 20;
    kprintf("First page alloc\nValue at %x is %d\n", p, *p);
    p = (uint64_t*)alloc_get_page();
    *p = 40;
    kprintf("Second page alloc\nValue at %x is %d\n", p, *p);

    // create a linked list with two struct pagelist_t
    kprintf("Third page alloc\n");
    struct pagelist_t* ele1 = (struct pagelist_t*)alloc_get_page();

    ele1->present = 1;
    ele1->next = NULL;
    struct pagelist_t* ele2 = (struct pagelist_t*)(ele1 + 1);
    ele2->present = 0;
    ele2->next = ele1;
    kprintf("addr of ele1 %p\naddr of ele2 %p\n ele2->ele1? %p\n", ele1, ele2,
            ele2->next);
    kprintf("ele1 present %d\nele2 present %d\n", ele1->present, ele2->present);
}

void
test_kmalloc_kfree()
{
    // basic tests for different data types
    int* p[257];
    int i;
    int loop_size = 0;
    for (i = 0; i < loop_size; i++) {
        p[i] = kmalloc(sizeof(int));
        kprintf("address %p\n", p[i]);
        *(p[i]) = i;
        kprintf("value %d\n", *(p[i]));
    }
    // simulate a call to alloc_get_page from a different function
    void* dummy = alloc_get_page();
    kprintf("Dummy page %p", dummy);
    for (i = 0; i < loop_size; i++) {
        kfree(p[i]);
    }
    for (i = 0; i < loop_size; i++) {
        p[i] = kmalloc(sizeof(int));
        kprintf("address %p\n", p[i]);
        *(p[i]) = i;
        kprintf("value %d\n", *(p[i]));
    }

    struct test_struct* struct_p[4096];
    for (i = 0; i < 3; i++) {
        struct_p[i] =
          (struct test_struct*)kmalloc(sizeof(struct test_struct) * 4096);
        kprintf("address %p\n", struct_p[i]);
        struct_p[i]->num = i;
        struct_p[i]->str = kmalloc(sizeof(char) * 20);
        (struct_p[i]->str)[0] = 'H';
        (struct_p[i]->str)[1] = 'e';
        (struct_p[i]->str)[2] = '\0';
        // strcpy(struct_p[i]->str, "Hello world!");
        kprintf("str = %s  num = %d\n", struct_p[i]->str, struct_p[i]->num);
    }
}

void
test_tasklist()
{
    task_struct s[3];
    s[0].pid = 1;
    s[1].pid = 2;
    s[2].pid = 3;
    s[3].pid = 4;

    tasklist_add_task(&s[0]);
    tasklist_add_task(&s[1]);
    tasklist_add_task(&s[2]);
    tasklist_add_task(&s[3]);

    kprintf("%d\n", tasklist_remove_task(2)); // Success
    kprintf("%d\n", tasklist_remove_task(2)); // Fail
    kprintf("%d\n", tasklist_remove_task(0)); // Fail

    while (1) {
        // Set character to pid number
        term_set_glyph(2, '0' + tasklist_schedule_task()->pid);
    }
}

void
test_sample_userspace_function()
{
    uint8_t i = 30;
    while (1) {
        i += 1;
        term_set_glyph(1, (char)i);
        task_yield();
        sleep(90);
        i %= 45;
    }
}

void
test_sample_thread_handler()
{
    while (1) {
        task_yield();
        term_set_glyph(0, '0' + task_get_this_task_struct()->pid);
        sleep(90);
    }
}

void
test_vma_list_and_page_fault()
{
    void* addr = (void*)((uint64_t)&_binary_tarfs_start +
                         3 * sizeof(struct posix_header_ustar));
    elf_read(addr, "/bin/ls");
    task_enter_ring3((void*)(task_get_this_task_struct()->entry_point));
}

void
test_sched()
{
    task_create(test_sample_thread_handler);
    task_create(test_sample_thread_handler);
    task_create(test_sample_thread_handler);
    task_create(test_sample_thread_handler);
    task_create(test_vma_list_and_page_fault);
    task_yield();
}

void
test_kprintf()
{
    for (int i = 0; i < 20; i++) {
        kprintf("\nTest\n");
        kprintf("more %d\n", i);
        kprintf("more %d\n", i);
    }
    kprintf("This is a sample text that is going to test my kprintf big time. "
            "Let's see if we can find any bugs. The hope is that there won't "
            "be any bugs and the code works as is without any problem. Now "
            "let's go catch'em all. We are the developers. We are the testers. "
            "One ring to rule them all. If you reading this much garbage, it "
            "is time to go work on OS");
}
