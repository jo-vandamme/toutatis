#ifndef __KERNEL_SYSCALL_H__
#define __KERNEL_SYSCALL_H__

void syscall_init(void);

#define DECL_SYSCALL0(fn) int syscall_##fn(void);
#define DECL_SYSCALL1(fn, P1) int syscall_##fn(P1);
#define DECL_SYSCALL2(fn, P1, P2) int syscall_##fn(P1, P2);
#define DECL_SYSCALL3(fn, P1, P2, P3) int syscall_##fn(P1, P2, P3);
#define DECL_SYSCALL4(fn, P1, P2, P3, P4) int syscall_##fn(P1, P2, P3, P4);
#define DECL_SYSCALL5(fn, P1, P2, P3, P4, P5) int syscall_##fn(P1, P2, P3, P4, P5);

#define DEFN_SYSCALL0(fn, num) \
    inline int syscall_##fn(void) { \
        int a; asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
        return a; \
    }

#define DEFN_SYSCALL1(fn, num, P1) \
    inline int syscall_##fn(P1 p1) { \
        int a; asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1)); \
        return a; \
    }

#define DEFN_SYSCALL2(fn, num, P1, P2) \
    inline int syscall_##fn(P1 p1, P2 p2) { \
        int a; asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2)); \
        return a; \
    }

#define DEFN_SYSCALL3(fn, num, P1, P2, P3) \
    inline int syscall_##fn(P1 p1, P2 p2, P3 p3) { \
        int a; asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d"((int)p3)); \
        return a; \
    }

#define DEFN_SYSCALL4(fn, num, P1, P2, P3, P4) \
    inline int syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4) { \
        int a; asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d"((int)p3), "S" ((int)p4)); \
        return a; \

#define DEFN_SYSCALL5(fn, num, P1, P2, P3, P4, P5) \
    inline int syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) { \
        int a; asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d"((int)p3), "S" ((int)p4), "D" ((int)p5)); \
        return a; \

DECL_SYSCALL0(thread_exit)
DECL_SYSCALL1(vga_print_str, const char *)
DECL_SYSCALL1(vga_print_dec, const uint32_t)
DECL_SYSCALL1(vga_print_hex, const uint32_t)

#endif
