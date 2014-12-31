#include <initrd.h>
#include <vfs.h>
#include <kheap.h>
#include <logging.h>
#include <multiboot.h>
#include <system.h>
#include <string.h>
#include <vsprintf.h>

static uintptr_t initrd_location;
static initrd_header_t *initrd_header;
static initrd_file_header_t *file_headers;

extern uint32_t kernel_voffset;
extern uintptr_t placement_address;

void initrd_parse();

void initrd_init(struct multiboot_info *mbi)
{
    assert(mbi->flags & MULTIBOOT_MODS);
    assert(mbi->mods_count > 0);

    uintptr_t initrd_end;
    initrd_location = *((uintptr_t *)mbi->mods_addr) + (uintptr_t)&kernel_voffset;
    initrd_end = *((uintptr_t *)(mbi->mods_addr + 4)) + (uintptr_t)&kernel_voffset;

    // TODO: free the memory between the kernel and the modules
    placement_address = initrd_end;

    initrd_header = ((initrd_header_t *)initrd_location);
    file_headers = (initrd_file_header_t *)(initrd_location + sizeof(initrd_header_t));

    kprintf(INFO, "[initrd] Location [0x%x-0x%x] = 0x%x bytes\n",
            initrd_location, initrd_end, initrd_end - initrd_location);
    
    assert(strcmp(initrd_header->magic, "INITRD") == 0);

    initrd_parse();
}

void initrd_parse()
{
    uint32_t i;
    kprintf(INFO, "[vfs] Processing initrd...\n");
    for (i = 0; i < initrd_header->nfiles; ++i) {
        kprintf(INFO, "\t-> Loading '%s': %d bytes...\n",
                file_headers[i].name, file_headers[i].length);
        file_headers[i].offset += initrd_location;
        char name[128];
        sprintf(name, "/%s", file_headers[i].name);
    }
}
