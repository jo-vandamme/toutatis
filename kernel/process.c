#include <system.h>
#include <kheap.h>
#include <paging.h>
#include <string.h>
#include <process.h>

extern page_dir_t *kernel_directory;

static uint32_t request_process_id()
{
    static uint32_t id = 0;
    return ++id;
}

process_t *create_process(const char name[64], uint32_t priority)
{
    irq_state_t irq_state = irq_save();

    process_t *process = (process_t *)kmalloc(sizeof(process_t));

    irq_restore(irq_state);

    if (!process) {
        return 0;
    }

    strncpy(process->name, name, sizeof(name));
    process->page_dir = clone_page_directory(kernel_directory);

    process->id = request_process_id();
    process->priority = priority;

    return process;
}

