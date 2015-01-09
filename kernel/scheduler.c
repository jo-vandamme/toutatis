#include <system.h>
#include <thread.h>
#include <paging.h>
#include <kheap.h>
#include <logging.h>
#include <scheduler.h>

int scheduling = 0;
thread_t *current_thread = 0;

extern page_dir_t *current_directory;

void schedule_thread(thread_t *thread)
{
    if (!thread) {
        return;
    }
    irq_state_t irq_state = irq_save();

    thread->next = thread->prev = 0;

    if (!current_thread) {
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
    if (!thread) {
        return;
    }
    irq_state_t irq_state = irq_save();

    if (thread->prev) {
        thread->prev->next = thread->next;
    }
    if (thread->next) {
        thread->next->prev = thread->prev;
    }

    irq_restore(irq_state);
}

static void switch_next(void)
{
    interrupt(IRQ(0));
}

inline static thread_t *next_ready_thread(void)
{
    if (current_thread) {
        return current_thread->next;
    }
    return 0;
}

uintptr_t schedule_tick(registers_t *regs)
{
    static uint64_t old_cycles_count = 0;
    uint64_t new_cycles_count;
    uintptr_t esp, old_esp = (uintptr_t)regs;

    if (old_cycles_count == 0) {
        old_cycles_count = get_cycles_count();
    }
    new_cycles_count = get_cycles_count();

    thread_t *next = next_ready_thread();

    if (next != current_thread) {
        /* register current esp and terminate task if needed */
        current_thread->esp = old_esp;
        if (current_thread->state == TASK_FINISHED) {
            unschedule_thread(current_thread);
            destroy_thread(current_thread);
        } else {
            current_thread->state = TASK_READY;
            current_thread->runtime += new_cycles_count - old_cycles_count;
        }
        /* switch to next task */
        esp = next->esp;
        next->state = TASK_RUNNING;
        current_thread = next;
        if (current_directory != current_thread->page_dir) {
            switch_page_directory(current_thread->page_dir);
        }
    } else {
        /* keep executing the same stuff */
        esp = old_esp;
    }

    old_cycles_count = get_cycles_count();

    return esp;
}

void scheduling_init(void)
{
    scheduling = 1;
}

void scheduling_finish(void)
{
    scheduling = 0;
}

uint32_t getpid(void)
{
    return current_thread != 0 ? current_thread->id : 0;
}
