#include <system.h>
#include <multiboot.h>
#include <vga.h>
#include <logging.h>
#include <paging.h>
#include <kheap.h>
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
        
    if (!(mbi->flags & MULTIBOOT_LOADER)) {
        kprintf(CRITICAL, "\033\014Bootloader isn't multiboot compliant\n");
        goto error;
    }
    kprintf(INFO, "Toutatis kernel booting from %s\n", (char *)(mbi->boot_loader_name + (uint32_t)&kernel_voffset));
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

    paging_init((mbi->mem_lower + mbi->mem_upper) * 1024);
    paging_mark_reserved((uint32_t)mbi - (uint32_t)&kernel_voffset);

    for (mmap_entry_t *mmap = (mmap_entry_t *)(mbi->mmap_addr + (uint32_t)&kernel_voffset);
        (uint32_t)mmap < mbi->mmap_addr + (uint32_t)&kernel_voffset + mbi->mmap_length;
        mmap = (mmap_entry_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size))) {
        if (mmap->type == 2) {
            for (uint64_t i = 0; i < mmap->length; i += FRAME_SIZE) {
                paging_mark_reserved((mmap->addr + i) & 0xfffff000);
            }
        }
    }
    paging_finalize();

    print_mmap(mbi);

    keyboard_init();

    void *p1 = kmalloc(8);
    void *p2 = kmalloc(8);
    *((char *)p1) = 'a';
    kprintf(INFO, "p1 @ 0x%x\n", (uint32_t)p1);
    kprintf(INFO, "p2 @ 0x%x\n", (uint32_t)p2);
    kfree(p2);
    kfree(p1);
    void *p3 = kmalloc(16);
    kprintf(INFO, "p3 @ 0x%x\n", (uint32_t)p3);

    //interrupt(19);
    
    uint8_t *ptr = (uint8_t *)(0xc0152000);
    uint8_t c = *ptr;
    *ptr = 1;
    (void)c;

    while (1) {
        uint8_t c = keyboard_lastchar();
        if (c == 'q') {
            kprintf(INFO, "reboot\n");
            arch_reset();
        }
    }

    //serial_terminate();
error:
    STOP;
}

void print_mmap(const multiboot_info_t *mbi)
{
    size_t size;

    kprintf(INFO, "Lower memory: %uKB - Upper memory: %uMB\n",
            mbi->mem_lower, mbi->mem_upper / 1024);

    mmap_entry_t * mmap = (mmap_entry_t *)(mbi->mmap_addr + (uint32_t)&kernel_voffset);
    uint32_t i = 1;

    while ((uint32_t)mmap < mbi->mmap_addr + (uint32_t)&kernel_voffset + mbi->mmap_length)
    {
        if (mmap->type > 4)
            continue;

        size = mmap->length / 1024;

        kprintf(INFO, "\033\012%02u: \033\016[%#020x\033\017:\033\016%#020x]\033\017 = %4u%s (%u - %s)\n",
                i++, (uint32_t)(mmap->addr), (uint32_t)(mmap->addr + mmap->length - 1), 
                (size > 1024) ? ((size / 1024 > 1024) ? size / 1024 / 1024 : size / 1024) : size,
                (size > 1024) ? ((size / 1024 > 1024) ? "GB" : "MB") : "KB",
                mmap->type, memory_types[mmap->type - 1]);

        mmap = (mmap_entry_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }
    kprintf(INFO, "\n");
}

