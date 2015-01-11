#ifndef __KERNEL_THREAD_H__
#define __KERNEL_THREAD_H__

#include <process.h>
#include <types.h>

struct page_dir;

typedef struct thread
{
    process_t       *process;  /* parent process */
    uintptr_t       esp;
    uint32_t        ss;
    uintptr_t       kstack;    /* kernel stack top */
    uintptr_t       ustack;    /* user stack top */
    struct page_dir *page_dir;
    uint32_t        id;
    int             state;
    uint32_t        priority;
    uint64_t        runtime;      
    struct thread   *next;
    struct thread   *prev;
} thread_t;

typedef void (*entry_t)();

uint32_t create_thread(process_t *process, entry_t entry, void *args, uint32_t priority, int user, int vm86);
void destroy_thread(thread_t *thread);
void thread_exit(void);
uint32_t get_num_threads(void);

void create_kernel_thread(void);

#endif
