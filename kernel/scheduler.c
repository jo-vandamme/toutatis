#include <system.h>
#include <thread.h>
#include <paging.h>
#include <kheap.h>
#include <logging.h>
#include <string.h>
#include <scheduler.h>

int scheduling = 0;
thread_t *current_thread = 0;
thread_t *kernel_thread = 0;

extern page_dir_t *current_directory;

void schedule_thread(thread_t *thread)
{
    if (!thread) {
        return;
    }
    irq_state_t irq_state = irq_save();

    thread->state = TASK_READY;
    thread->next = thread->prev = 0;

    if (!kernel_thread) {
        kernel_thread = thread;
        current_thread = thread;
        current_thread->next = current_thread->prev = thread;
    } else {
        thread->prev = current_thread->prev;
        thread->next = current_thread;
        current_thread->prev->next = thread;
        current_thread->prev = thread;
    }

    irq_restore(irq_state);
}

void unschedule_thread(struct thread *thread)
{
    if (!thread || thread == kernel_thread) {
        return;
    }
    //DBPRINT(" unsched  ");
    irq_state_t irq_state = irq_save();

    if (thread->prev) {
        thread->prev->next = thread->next;
    }
    if (thread->next) {
        thread->next->prev = thread->prev;
    }

    irq_restore(irq_state);
}


void switch_next(void)
{
    irq_enable();
    interrupt(IRQ(0));
}

inline static thread_t *next_ready_thread(void)
{
    if (!current_thread) {
        return 0;
    }
    thread_t *thread = current_thread->next;
    while (thread && thread->state == TASK_SLEEP) {
        thread = thread->next;
    }
    return thread;
}

uintptr_t schedule_tick(registers_t *regs)
{
    //DBPRINT("switch ");
    irq_state_t irq_state = irq_save();

    static uint64_t old_cycles_count = 0;
    uint64_t new_cycles_count;
    uintptr_t esp, old_esp = (uintptr_t)regs;

    if (old_cycles_count == 0) {
        old_cycles_count = get_cycles_count();
    }
    new_cycles_count = get_cycles_count();

    thread_t *next = next_ready_thread();

    /*uintptr_t esp_;
    asm volatile("mov %%esp, %0" : "=r" (esp_));
    DBPRINT("current esp = %x\n", esp_);
    */

    //DBPRINT("- cur:%x next:%x ", current_thread, next);

    if (next && next != current_thread) {

        /* register current esp and terminate task if needed */
        if (current_thread->state == TASK_FINISHED) {
            unschedule_thread(current_thread);
            destroy_thread(current_thread);
        } else {
            current_thread->esp = old_esp;
            current_thread->state = TASK_READY;
            current_thread->runtime += new_cycles_count - old_cycles_count;
        }

        /* switch to next task */
        esp = next->esp;
        set_kernel_stack(esp);
        next->state = TASK_RUNNING;
        current_thread = next;
        if (current_directory != current_thread->page_dir) {
            DBPRINT("page dir: %x -> %x nthreads:%d\n", 
                    current_directory, current_thread->page_dir, get_num_threads());
            switch_page_directory(current_thread->page_dir);
        }

    } else {
        /* keep executing the same stuff */
        esp = old_esp;
    }
    //DBPRINT("- esp: %x->%x\n", old_esp, esp);

    old_cycles_count = get_cycles_count();

    irq_restore(irq_state);

    return esp;
}

void scheduling_init(void)
{
    irq_state_t irq_state = irq_save();
    if (!current_thread) {
        create_kernel_thread();
    }
    scheduling = 1;
    irq_restore(irq_state);
}

void scheduling_finish(void)
{
    scheduling = 0;
}

uint32_t getpid(void)
{
    return current_thread != 0 ? current_thread->id : 0;
}
