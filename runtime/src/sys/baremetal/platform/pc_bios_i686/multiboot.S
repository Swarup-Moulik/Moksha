.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .text
.global _start
_start:
#if defined(__x86_64__)
    /* 64-bit mode: Use 64-bit stack pointer */
    mov $_estack, %rsp
#else
    /* 32-bit mode: Use 32-bit stack pointer */
    mov $_estack, %esp
#endif

    .extern main
    call main

    /* Send QEMU ACPI Shutdown Signal */
    mov $0x2000, %ax
    mov $0x604, %dx
    outw %ax, %dx

    /* Fallback halt if not running in QEMU */
    cli
1:  hlt
    jmp 1b
