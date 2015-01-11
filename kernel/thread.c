#include <system.h>
#include <process.h>
#include <kheap.h>
#include <scheduler.h>
#include <string.h>
#include <paging.h>
#include <thread.h>

#define STACK_SIZE 0x2000
#define stack_top(s) ((s) + STACK_SIZE - 0x000)

uint32_t num_threads = 0;
extern thread_t *current_thread;
extern page_dir_t *current_directory;
extern page_dir_t *kernel_directory;

static uint32_t request_thread_id()
{
    static uint32_t id = 0;
    return ++id;
}

uint32_t get_num_threads()
{
    return num_threads;
}

void create_kernel_thread(void)
{
    irq_state_t irq_state = irq_save();

    thread_t *thread = (thread_t *)kmalloc(sizeof(thread_t));
    if (!thread) {
        return;
    }
    memset(thread, 0, sizeof(thread_t));

    thread->id = request_thread_id();
    thread->process = 0;
    thread->page_dir = current_directory;

    ++num_threads;

    schedule_thread(thread);

    irq_restore(irq_state);
}

#define PUSH(stack, x) (*--(stack) = x)

uint32_t create_thread(process_t *process, entry_t entry, void *args, uint32_t priority, int user, int vm86)
{
    /* create thread */
    thread_t *thread = (thread_t *)kmalloc(sizeof(thread_t));
    if (!thread) {
        DBPRINT("- !thread");
        return 0;
    }
    memset(thread, 0, sizeof(thread));

    /* setup the stack(s) */
    thread->kstack = (uintptr_t)kmalloc(STACK_SIZE);
    if (!thread->kstack) {
        DBPRINT("- !kstack");
        return 0;
    }
    if (user) {
        thread->ustack = (uintptr_t)kmalloc(STACK_SIZE);
        if (!thread->ustack) {
            DBPRINT("- !ustack");
            return 0;
        }
    } else {
        thread->ustack = 0;
    }

    uint32_t *kstack = (uint32_t *)stack_top(thread->kstack);
    uint32_t *ustack = (uint32_t *)stack_top(thread->ustack);

    uint32_t data_segment = user ? 0x20+3 : 0x10;
    uint32_t code_segment = user ? 0x18+3 : 0x08;
    /* bit 2 always set - bit 9 = interruption flag (IF) - bits 12/13 = privilege level 
     * bit 17 = virtua-8086 mode (VM) */
    uint32_t eflags = (user ? 0x3202 : 0x0202) | (vm86 ? 1 << 17 : 0 << 17);

    if (user) {
        PUSH(ustack, (uintptr_t)args);      /* args */
        PUSH(ustack, 0xdeadcaca);           /* return address - thread should be finished
                                             * with a system call, not by jumping to this address */
        PUSH(kstack, 0x23);                 /* ss */
        PUSH(kstack, stack_top(thread->ustack)); /* esp */
    } else {
        PUSH(kstack, (uintptr_t)args);      /* args */
        PUSH(kstack, (uintptr_t)&thread_exit); /* return address */
    }
    PUSH(kstack, eflags);                   /* eflags */
    PUSH(kstack, code_segment);             /* cs */
    PUSH(kstack, (uintptr_t)entry);         /* eip */
    PUSH(kstack, 0);                        /* error code */
    PUSH(kstack, 0);                        /* interrupt number */
    PUSH(kstack, 0);                        /* eax */
    PUSH(kstack, 0);                        /* ebx */
    PUSH(kstack, 0);                        /* ecx */
    PUSH(kstack, 0);                        /* edx */
    PUSH(kstack, 0);                        /* esi */
    PUSH(kstack, 0);                        /* edi */
    PUSH(kstack, 0);                        /* ebp */
    PUSH(kstack, data_segment);             /* ds */
    PUSH(kstack, data_segment);             /* es */
    PUSH(kstack, data_segment);             /* fs */
    PUSH(kstack, data_segment);             /* gs */

    thread->esp = (uintptr_t)kstack;
    thread->ss = data_segment;
    thread->id = request_thread_id();
    thread->process = process;
    /* thread's priority can't exceed its parent's priority */
    thread->priority = min(priority, process->priority); 
    thread->page_dir = process ? process->page_dir : kernel_directory;
    thread->runtime = 0;

    irq_state_t irq_state = irq_save();

    //if (user) {
        set_kernel_stack((uintptr_t)kstack);
    //}
    ++num_threads;

    /* register this thread */
    schedule_thread(thread);

    irq_restore(irq_state);

    return thread->id;
}

void destroy_thread(thread_t *thread)
{
    if (!thread) {
        return;
    }
    --num_threads;

    kfree((void *)thread->kstack);
    if (thread->ustack) {
        kfree((void *)thread->ustack);
    }
    kfree(thread);
}

void thread_exit(void)
{
    DBPRINT("thread_exit ");
    //irq_state_t irq_state = irq_save();
    irq_disable();
    current_thread->state = TASK_FINISHED;
    //irq_restore(irq_state);
    irq_enable();

    //DBPRINT("calling switch\n");
    switch_next();

    /* the thread will be removed during the next task switch */
    for (;;) 
        halt();
}

