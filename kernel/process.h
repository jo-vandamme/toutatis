#ifndef __KERNEL_PROCESS_H__
#define __KERNEL_PROCESS_H__

#include <types.h>

#define TASK_SLEEP    0
#define TASK_READY    1
#define TASK_RUNNING  2
#define TASK_FINISHED 3

struct thread;
struct process;
struct page_dir;

typedef struct process
{
    char            name[64];
    uint32_t        id;
    uint32_t        priority;
    int             state;
    struct page_dir *page_dir;
    struct process  *next;
    struct process  *prev;
    struct thread   *threads;
} process_t;

process_t *create_process(const char name[64], uint32_t priority);

#endif
