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
#include <string.h>
#include <process.h>
#include <syscall.h>

uint32_t x, y, z, w;
uint32_t xorshift128(void)
{
    uint32_t t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) ^ t ^ (t >> 8);
}

void print_mmap(const struct multiboot_info *mbi);

static device_t *vga_driver;
static device_t *com_driver;

extern uint32_t kernel_voffset;
extern uint32_t kernel_start;
extern uint32_t kernel_end;

uintptr_t stack_start;
size_t stack_size;

unsigned char alph[] = "abcdefghijklmnopqrstuvwxyz";

void func1(unsigned int off)
{
    int a = 0;
    syscall_vga_print_str(".");
    for (unsigned i = 0; i < 5000000; ++i) {
        ++a;
        //if (i % 1000 == 0) syscall_vga_print_str(".");
    }
    syscall_thread_exit();
}

void func2(unsigned int off)
{
    x = 18;
    y = 43;
    z = 1118;
    w = 8932;
    uint16_t *video = (uint16_t *)(0xc00b8000 + off);
    uint32_t i = 0, i_max = xorshift128();
    i_max = 1000000;
    for (i = 0; i < i_max; ++i)
        *video = (uint16_t)alph[i++ % sizeof(alph)] | 0x0f00;
}

void reset()
{
    for (;;) {
        uint8_t c = keyboard_lastchar();
        if (c == 'r') {
            arch_reset();
        }
    }
}

char *memory_types[] =
{
    "Available",
    "Reserved",
    "ACPI Reclaim",
    "ACPI NVS Memory"
};

void main(uint32_t magic, struct multiboot_info *mbi, 
          uintptr_t esp, uintptr_t stack_end)
{
    stack_start = esp;
    stack_size = stack_end - stack_start;

    vga_driver = vga_init();
    com_driver = serial_init();
    logging_init(vga_driver, com_driver);
        
    assert(magic == MULTIBOOT_MAGIC);
    assert(mbi->flags & MULTIBOOT_LOADER);
    kprintf(INFO, "\033\012Toutatis kernel booting from %s\033\017\n", 
            (char *)(mbi->boot_loader_name + (uint32_t)&kernel_voffset));

    arch_init();

    initrd_init(mbi); 

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

    void *p1 = kmalloc(8);
    void *p2 = kmalloc(8);
    *((char *)p1) = 'a';
    kprintf(INFO, "p1 @ 0x%x\n", (uint32_t)p1);
    kprintf(INFO, "p2 @ 0x%x\n", (uint32_t)p2);
    kfree(p2);
    kfree(p1);
    void *p3 = kmalloc(16);
    kprintf(INFO, "p3 @ 0x%x\n", (uint32_t)p3);
    uintptr_t phys;
    void *p4 = kmalloc_ap(0x1a0000, &phys);
    memset(p4, 0, 0x1a0000);
    *((char *)p4) = 'z';
    kprintf(INFO, "p4 @ 0x%x phys = %x\n", (uint32_t)p4, phys);
    void *p5 = kmalloc(0x02);
    kprintf(INFO, "p5 @ 0x%x\n", (uint32_t)p5);
    kfree(p5);
    kfree(p4);
    kfree(p3);

    print_mmap(mbi);

    syscall_init();

    multitasking_init();

    keyboard_init();

    unsigned int off = 0;
    process_t *proc = create_process("Process 1", 1);
    create_thread(proc, func2, (void *)(off +  0), 1, 0, 0);
    create_thread(proc, func2, (void *)(off +  1), 1, 0, 0);
    create_thread(proc, func2, (void *)(off +  2), 1, 0, 0);
    create_thread(proc, func2, (void *)(off +  3), 1, 0, 0);
    create_thread(proc, func2, (void *)(off +  4), 1, 0, 0);
    create_thread(proc, func2, (void *)(off +  5), 1, 0, 0);
    create_thread(proc, func2, (void *)(off +  6), 1, 0, 0);
    create_thread(proc, func2, (void *)(off +  7), 1, 0, 0);
    create_thread(proc, func2, (void *)(off +  8), 1, 0, 0);
    create_thread(proc, func2, (void *)(off +  9), 1, 0, 0);
    create_thread(proc, func2, (void *)(off + 10), 1, 0, 0);
    create_thread(proc, func2, (void *)(off + 11), 1, 0, 0);
    create_thread(proc, func2, (void *)(off + 12), 1, 0, 0);
    create_thread(proc, func2, (void *)(off + 13), 1, 0, 0);
    sleep(5);
    create_thread(proc, func2, (void *)(off + 14), 1, 0, 0);
    create_thread(proc, func2, (void *)(off + 15), 1, 0, 0);
    create_thread(proc, func2, (void *)(off + 16), 1, 0, 0);
    create_thread(proc, func2, (void *)(off + 17), 1, 0, 0);
    create_thread(proc, func2, (void *)(off + 18), 1, 0, 0);
    create_thread(proc, func2, (void *)(off + 19), 1, 0, 0);
    create_thread(proc, func2, (void *)(off + 20), 1, 0, 0);
    create_thread(proc, func2, (void *)(off + 21), 1, 0, 0);
    create_thread(proc, func2, (void *)(off + 22), 1, 0, 0);
    create_thread(proc, func2, (void *)(off + 23), 1, 0, 0);
    create_thread(proc, func2, (void *)(off + 24), 1, 0, 0);
    create_thread(proc, func2, (void *)(off + 25), 1, 0, 0);

    for (;;);
/*
    unsigned int i = 0;
    while (1) {
        sleep(2);

        uint16_t *video = (uint16_t *)(0xc00b8000 + 80);
        *video = (uint16_t)alph[i++ % sizeof(alph)] | 0x0f00;
        
        create_thread(proc, func2, (void *)(off + 500), 1, 0, 0);
        off += 2;
        off %= (80 * 25);
        if (keyboard_lastchar() == 'u') {
            create_thread(proc, func1, (void *)0, 1, 1, 0);
        }
    }

    //serial_terminate();
    stop();
    */
}

void print_mmap(const struct multiboot_info *mbi)
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

