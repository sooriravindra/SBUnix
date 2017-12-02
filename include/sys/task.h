#include <sys/vfs.h>
#ifndef _TASK_H
#define _TASK_H
#include <sys/vma.h>
#define TASK_FILETABLE_SIZE 64
#define PATH_LENGTH 32
/*
 * Don't change the order of members of the following structures
 * Context switch might fail
 */
typedef struct _regstruct
{
    uint64_t rbp;
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t flags;
    uint64_t cr3;
    uint64_t rsp;
} __attribute__((packed)) __attribute__((aligned(64))) reg_struct;

typedef struct _taskstruct
{
    reg_struct regs;
    pid_t pid;
    pid_t ppid;
    void* stack_page;
    vfs_file_object* filetable[TASK_FILETABLE_SIZE];
    struct vma_struct* vma_list;
    char binary_name[PATH_LENGTH];
    uint64_t entry_point;
    char cwd[PATH_LENGTH];
} __attribute__((packed)) __attribute__((aligned(64))) task_struct;

task_struct* task_create();
void task_destroy(task_struct* t);
void task_yield();
void task_exec_ring3(char* bin_name, char** argv, char** envp);
task_struct* task_get_this_task_struct();
uint64_t task_initial_setup();
void task_yield();
void task_save_state();
#endif
