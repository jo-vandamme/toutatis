#ifndef __KERNEL_SCHEDULER_H__
#define __KERNEL_SCHEDULER_H__

struct thread;
struct registers;

void schedule_thread(struct thread *thread);
void unschedule_thread(struct thread *thread);

uintptr_t schedule_tick(struct registers *regs);

void scheduling_init(void);
void scheduling_finish(void);

uint32_t getpid(void);

#endif
