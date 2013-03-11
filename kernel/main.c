#include <arch.h>
#include <multiboot.h>
#include <vga.h>
#include <logging.h>
#include <phys_mem.h>
#include <serial.h>
#include <driver.h>

void print_banner(const char *loader);
void print_mmap(const multiboot_info_t *mbi);
void my_func(registers_t *r);
void my_func2(registers_t *r);
void my_func3(registers_t *r);
void wait(u32_t ms);

static device_t *vga_driver;
static device_t *com_driver;

char *memory_types[] =
{
        "Available",
        "Reserved",
        "ACPI Reclaim",
        "ACPI NVS Memory"
};

void main(u32_t magic, multiboot_info_t *mbi, u32_t kernel_size)
{
        vga_driver = vga_init();
        com_driver = serial_init();
        logging_init(vga_driver, com_driver);

        if (mbi->flags & MULTIBOOT_LOADER) {
                print_banner((char *)mbi->boot_loader_name);
        }
        if (magic != MULTIBOOT_MAGIC) {
                kprintf("\033\014Bad magic number %#010x\n", magic);
                goto error;
        }
        if (!(mbi->flags & MULTIBOOT_MEMINFO) || !(mbi->flags & MULTIBOOT_MMAP)) {
                kprintf("\033\014No memory information\n");
                goto error;
        }

        arch_init();

        print_mmap(mbi);
        pmm_init(mbi, (mbi->mem_upper + mbi->mem_lower) * 1024, kernel_size);

        serial_terminate();
error:
        for (;;) ;
}

void print_banner(const char *loader)
{
        kprintf("Toutatis kernel booting from %s\n", loader);
}

void print_mmap(const multiboot_info_t *mbi)
{
        unsigned long size;

        kprintf("\033\010Lower memory: \033\007%uKB\033\010 - Upper memory: \033\007%uMB\033\010\n",
                mbi->mem_lower, mbi->mem_upper / 1024);

        mmap_entry_t * mmap = (mmap_entry_t *)mbi->mmap_addr;
        u32_t i = 1;
        while ((u32_t)mmap < mbi->mmap_addr + mbi->mmap_length)
        {
                if (mmap->type > 4)
                        mmap->type = 1;

                size = mmap->length_low / 1024;

                kprintf("\033\012%02u: \033\016%#010x%010x\033\010:\033\016%#010x%010x"
                        "\033\010 -> %4u%s \033\010(%u - \033\007%s\033\010)\n",
                        i++, mmap->addr_high, mmap->addr_low,
                        mmap->length_high, mmap->length_low,
                        (size > 1024) ? ((size / 1024 > 1024) ? size / 1024 / 1024 : size / 1024) : size,
                        (size > 1024) ? ((size / 1024 > 1024) ? "GB" : "MB") : "KB",
                        mmap->type, memory_types[mmap->type - 1]);

                mmap = (mmap_entry_t *)((u32_t)mmap + mmap->size + sizeof(mmap->size));
        }
        kprintf("\033\007\n");
}

