#include <system.h>
#include <multiboot.h>
#include <vga.h>
#include <logging.h>
#include <paging.h>
#include <kheap.h>
#include <serial.h>
#include <driver.h>
#include <keyboard.h>
#include <initrd.h>
#include <vfs.h>

void print_mmap(const multiboot_info_t *mbi);

static device_t *vga_driver;
static device_t *com_driver;

extern uint32_t kernel_voffset;
extern uint32_t kernel_start;
extern uint32_t kernel_end;

extern uintptr_t placement_address;

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
        
    assert(magic == MULTIBOOT_MAGIC);
    assert(mbi->flags & MULTIBOOT_LOADER);
    kprintf(INFO, "Toutatis kernel booting from %s\n", 
            (char *)(mbi->boot_loader_name + (uint32_t)&kernel_voffset));

    /* find location of the initial ramdisk */
    assert(mbi->flags & MULTIBOOT_MODS);
    assert(mbi->mods_count > 0);
    uintptr_t initrd_start = *((uintptr_t *)mbi->mods_addr) + (uintptr_t)&kernel_voffset;
    uintptr_t initrd_end = *((uintptr_t *)(mbi->mods_addr + 4)) + (uintptr_t)&kernel_voffset;
    // TODO: free the memory between the kernel and the modules
    kprintf(INFO, "placement address = 0x%x\n", placement_address);
    placement_address = initrd_end;
    kprintf(INFO, "Initrd: start @ 0x%x end @ 0x%x len = 0x%x\n",
            initrd_start, initrd_end, initrd_end - initrd_start);
    vnode_t *node = initrd_init(initrd_start); 
    (void)node;

    kprintf(DEBUG, "kernel start: %#010x\nkernel end: %#010x\nkernel size: %u bytes\n", 
            (uint32_t)&kernel_start, (uint32_t)&kernel_end, 
            (uint32_t)&kernel_end - (uint32_t)&kernel_start);
        
    arch_init();

    assert(mbi->flags & MULTIBOOT_MEMINFO);
    paging_init((mbi->mem_lower + mbi->mem_upper) * 1024);
    paging_mark_reserved((uint32_t)mbi - (uint32_t)&kernel_voffset);

    assert(mbi->flags & MULTIBOOT_MMAP);
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
    void *p4 = kmalloc_a(0x1a0000);
    *((char *)p4) = 'z';
    kprintf(INFO, "p4 @ 0x%x\n", (uint32_t)p4);
    void *p5 = kmalloc(0x02);
    kprintf(INFO, "p5 @ 0x%x\n", (uint32_t)p5);
    kfree(p5);
    kfree(p4);
    kfree(p3);

    while (1) {
        uint8_t c = keyboard_lastchar();
        if (c == 'q') {
            kprintf(INFO, "reboot\n");
            arch_reset();
        }
    }

    //serial_terminate();
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

