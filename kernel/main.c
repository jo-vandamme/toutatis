#include <arch.h>
#include <multiboot.h>
#include <vga.h>
#include <kprintf.h>
#include <phys_mem.h>

void print_banner(const char *loader);
void print_mmap(const multiboot_info_t *mbi);
void my_func(registers_t *r);
void my_func2(registers_t *r);
void my_func3(registers_t *r);
void wait(u32_t ms);

char *memory_types[] =
{
        "Available",
        "Reserved",
        "ACPI Reclaim",
        "ACPI NVS Memory"
};

void main(u32_t magic, multiboot_info_t *mbi, u32_t kernel_size)
{
        vga_init();

        print_banner((char *)mbi->boot_loader_name);

        if (magic != MB_MAGIC) {
                kprintf("\033\014Bad magic number %#010x\n", magic);
                goto error;
        }
        print_mmap(mbi);

        arch_init();
        (void)kernel_size;
        pmm_init(mbi, (mbi->mem_upper + mbi->mem_lower) * 1024, kernel_size);

        wait(1000);
        attach_interrupt_handler(IRQ(0), my_func);
        sleep(500);
        attach_interrupt_handler(IRQ(0), my_func2);
        sleep(500);
        detach_interrupt_handler(IRQ(0), my_func2);
        sleep(300);
        detach_interrupt_handler(IRQ(0), my_func);
        newline;

        attach_interrupt_handler(IRQ(1), my_func3);
        enable_irq(1);

error:
        for (;;) ;
}

void print_banner(const char *loader)
{
        vga_clear();
        kprintf("\n  Toutatis kernel booting from %s\n"
                "\033\016  ------------------------------------------------\033\007\n", loader);
}

void print_mmap(const multiboot_info_t *mbi)
{
        unsigned long size;

        kprintf("\n\t\t\tBIOS memory map:\n"
                "\033\007\t\t\t----------------\n"
                "\033\010\tLower memory: \033\007%uKB\033\010 - Upper memory: \033\007%uMB\033\010\n\n",
                mbi->mem_lower, mbi->mem_upper / 1024);

        mmap_entry_t * mmap = (mmap_entry_t *)mbi->mmap_addr;
        u32_t i = 1;
        while ((u32_t)mmap < mbi->mmap_addr + mbi->mmap_length)
        {
                if (mmap->type > 4)
                        mmap->type = 1;

                size = ((mmap->length_high << 8) + mmap->length_low) / 1024;

                kprintf("\t\033\012%02u\033\012: \033\016%#010x\033\010:\033\016%#010x"
                        "\033\010 -> %4u%s \033\010(%u - \033\007%s\033\010)\n",
                        i++, (mmap->addr_high << 8) + mmap->addr_low,
                        (mmap->addr_high << 8) + (mmap->length_high << 8)
                        + mmap->addr_low + mmap->length_low - 1,
                        (size > 1024) ? ((size / 1024 > 1024) ? size / 1024 / 1024 : size / 1024) : size,
                        (size > 1024) ? ((size / 1024 > 1024) ? "GB" : "MB") : "KB",
                        mmap->type, memory_types[mmap->type - 1]);

                mmap = (mmap_entry_t *)((u32_t)mmap + 24);
        }
        vga_print_str("\033\007\n");
}

void my_func(registers_t *r)
{
        (void)r;
        kprintf("\033\012\r%u", get_ticks_count());
}

void my_func2(registers_t *r)
{
        (void)r;
        kprintf("\t%x", get_ticks_count());
}

void my_func3(registers_t *r)
{
        (void)r;
        kprintf("\033\013#");
}

void wait(u32_t ms)
{
        static char *wait = "|/-\\";
        static u8_t i = 0;
        u32_t current = get_ticks_count();

        while (current + ms > get_ticks_count()) {
                sleep(50);
                kprintf("\r%c", *(wait + (i++) % 4));
        }
}
