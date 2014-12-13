#include <arch.h>
#include <multiboot.h>
#include <vga.h>
#include <logging.h>
#include <phys_mem.h>
#include <virt_mem.h>
#include <serial.h>
#include <driver.h>
#include <keyboard.h>

void print_mmap(const multiboot_info_t *mbi);

static device_t *vga_driver;
static device_t *com_driver;

extern uint32_t kernel_voffset;
extern uint32_t kernel_start;
extern uint32_t kernel_end;

char *memory_types[] =
{
    "Available",
    "Reserved",
    "ACPI Reclaim",
    "ACPI NVS Memory"
};

void main(uint32_t magic, multiboot_info_t *mbi)
{
    vga_driver = vga_init();
    com_driver = serial_init();
    logging_init(vga_driver, com_driver);
        
    if (mbi->flags & MULTIBOOT_LOADER) {
        kprintf(INFO, "Toutatis kernel booting from %s\n", (char *)(mbi->boot_loader_name + (uint32_t)&kernel_voffset));
    }
    if (magic != MULTIBOOT_MAGIC) {
        kprintf(CRITICAL, "\033\014Bad magic number %#010x\n", magic);
        goto error;
    }
    if (!(mbi->flags & MULTIBOOT_MEMINFO) || !(mbi->flags & MULTIBOOT_MMAP)) {
        kprintf(CRITICAL, "\033\014No memory information\n");
        goto error;
    }

    kprintf(DEBUG, "kernel start: %#010x\nkernel end: %#010x\nkernel size: %u bytes\n", 
            (uint32_t)&kernel_start, (uint32_t)&kernel_end, (uint32_t)&kernel_end - (uint32_t)&kernel_start);
        
    arch_init();

    if (pmm_init(mbi))
        goto error;

    vmm_init();

    keyboard_init();
    print_mmap(mbi);

    //interrupt(19);
    
    uint8_t *ptr = (uint8_t *)(0xc03fffff + 1);
    uint8_t c = *ptr;
    *ptr = 1;
    (void)c;

    serial_terminate();
error:
    for (;;) ;
}

void print_mmap(const multiboot_info_t *mbi)
{
    unsigned long size;

    kprintf(INFO, "Lower memory: %uKB - Upper memory: %uMB\n",
            mbi->mem_lower, mbi->mem_upper / 1024);

    mmap_entry_t * mmap = (mmap_entry_t *)(mbi->mmap_addr + (uint32_t)&kernel_voffset);
    uint32_t i = 1;
    while ((uint32_t)mmap < mbi->mmap_addr + (uint32_t)&kernel_voffset + mbi->mmap_length)
    {
        if (mmap->type > 4)
            continue;

        size = mmap->length_low / 1024;

        kprintf(INFO,
                "\033\012%02u: \033\016%#010x%010x\033\017:\033\016%#010x%010x\033\017-> %4u%s (%u - %s)\n",
                i++, mmap->addr_high, mmap->addr_low, mmap->addr_high + mmap->length_high, 
                mmap->addr_low + mmap->length_low-1,
                (size > 1024) ? ((size / 1024 > 1024) ? size / 1024 / 1024 : size / 1024) : size,
                (size > 1024) ? ((size / 1024 > 1024) ? "GB" : "MB") : "KB",
                mmap->type, memory_types[mmap->type - 1]);

        mmap = (mmap_entry_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }
    kprintf(INFO, "\n");
}

