.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

/* Multiboot1 header */
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss
.align 4096
pml4_table:
    .skip 4096
pdpt_table:
    .skip 4096
pd_table:
    .skip 4096

.section .rodata
.align 8
gdt64:
    .quad 0 # Null descriptor
gdt64_code:
    /* 64-bit Code Segment */
    .quad (1<<43) | (1<<44) | (1<<47) | (1<<53)
gdt64_data:
    /* 64-bit Data Segment */
    .quad (1<<41) | (1<<44) | (1<<47)
gdt64_pointer:
    .word . - gdt64 - 1
    .long gdt64

.section .text
.code32
.global _start
_start:
    /* 1. Setup temporary 32-bit stack */
    mov $_estack, %esp

    /* 2. Zero-out Page Tables */
    mov $pml4_table, %edi
    mov $3072, %ecx
    xor %eax, %eax
    rep stosl

    /* 3. Build Identity Map (First 8MB mapped using 4x 2MB Large Pages) */

    /* PML4[0] -> PDPT */
    mov $pdpt_table, %eax
    or $0x3, %eax
    mov %eax, pml4_table

    /* PDPT[0] -> PD */
    mov $pd_table, %eax
    or $0x3, %eax
    mov %eax, pdpt_table

    /* PD[0] -> 0x000000 (Covers VGA buffer & BIOS) */
    mov $0x83, %eax
    mov %eax, pd_table

    /* PD[1] -> 0x200000 (Covers our Kernel Code & Data!) */
    mov $0x200083, %eax
    mov %eax, pd_table + 8

    /* PD[2] -> 0x400000 (Covers BSS and Heap) */
    mov $0x400083, %eax
    mov %eax, pd_table + 16

    /* PD[3] -> 0x600000 (Covers Stack and more Heap) */
    mov $0x600083, %eax
    mov %eax, pd_table + 24

    /* 4. Load PML4 into CR3 */
    mov $pml4_table, %eax
    mov %eax, %cr3

    /* 5. Enable PAE in CR4 */
    mov %cr4, %eax
    or $0x20, %eax
    mov %eax, %cr4

    /* 6. Enable Long Mode in EFER MSR */
    mov $0xC0000080, %ecx
    rdmsr
    or $0x100, %eax
    wrmsr

    /* 7. Enable Paging in CR0 */
    mov %cr0, %eax
    or $0x80000001, %eax
    mov %eax, %cr0

    /* 8. Load 64-bit GDT */
    lgdt gdt64_pointer

    /* 9. Far Jump to 64-bit Long Mode Segment (Safe Method) */
    pushl $0x08
    pushl $long_mode_entry
    lret

.code64
long_mode_entry:
    /* Set 64-bit Segment Registers */
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    /* Setup 64-bit Stack Pointer */
    mov $_estack, %rsp

    /* Jump into Moksha Kernel main */
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
