#include <system.h>
//#include <logging.h>
#include <vga.h>
#include <syscall.h>

DEFN_SYSCALL1(vga_print_str, 0, const char *)
DEFN_SYSCALL1(vga_print_dec, 1, const uint32_t)
DEFN_SYSCALL1(vga_print_hex, 2, const uint32_t)

static void syscall_handler(registers_t *regs);

uint32_t num_syscalls = 3;

static uintptr_t syscalls[] = 
{
    (uintptr_t)&vga_print_str,
    (uintptr_t)&vga_print_dec,
    (uintptr_t)&vga_print_hex
};

void syscall_init()
{
    attach_interrupt_handler(SYSCALL_VECTOR, &syscall_handler);
}

void syscall_handler(registers_t *regs)
{
    if (regs->eax >= num_syscalls) {
        return;
    }

    uintptr_t syscall = syscalls[regs->eax];

    int ret;
    asm volatile (
            "push %1    \n"
            "push %2    \n"
            "push %3    \n"
            "push %4    \n"
            "push %5    \n"
            "call *%6   \n"
            "pop %%ebx  \n"
            "pop %%ebx  \n"
            "pop %%ebx  \n"
            "pop %%ebx  \n"
            "pop %%ebx  \n"
            : "=a"(ret) : "r"(regs->edi), "r"(regs->esi), "r"(regs->edx), 
                          "r"(regs->ecx), "r"(regs->ebx), "r"(syscall));
    regs->eax = ret;
}
