#if defined(__aarch64__)
// ============================================================================
// AArch64 (ARM64) QEMU Virt Startup Code
// ============================================================================
.section .text.boot, "ax"
.global _start
_start:
    /* 1. Ensure we run on Core 0 only */
    mrs x0, mpidr_el1
    mov x1, #0xFF
    and x0, x0, x1
    cbnz x0, halt

    /* 2. Check current Exception Level (EL) */
    mrs x0, CurrentEL
    lsr x0, x0, #2
    and x0, x0, #0x3
    cmp x0, #2
    b.ne el1_entry

    /* Drop from EL2 to EL1 if QEMU started us in EL2 */
    mov x0, #(1 << 31)         /* Execution state for EL1 is AArch64 */
    msr hcr_el2, x0
    mov x0, #0x3C5             /* EL1h mode with masked interrupts */
    msr spsr_el2, x0
    adr x0, el1_entry
    msr elr_el2, x0
    eret

el1_entry:
    /* 3. Enable FP / SIMD (NEON) at EL1 */
    mrs x0, cpacr_el1
    orr x0, x0, #(0x3 << 20)   /* Bits [21:20] = 0b11 (Full access to FP/SIMD) */
    msr cpacr_el1, x0
    isb

    /* 4. Set up Stack Pointer */
    ldr x0, =_estack
    mov sp, x0

    /* 5. Clear BSS section safely (4-byte alignment safe) */
    ldr x1, =_sbss
    ldr x2, =_ebss
bss_loop:
    cmp x1, x2
    b.ge bss_done
    str wzr, [x1], #4
    b bss_loop
bss_done:

    /* 6. Jump to main */
    bl main

    /* 7. Power off the machine via PSCI SYSTEM_OFF */
    ldr x0, =0x84000008    /* PSCI SYSTEM_OFF function ID */
    hvc #0                 /* Hypervisor Call to power off */

halt:
    wfe
    b halt


#elif defined(__riscv)
// ============================================================================
// RISC-V (32-bit & 64-bit) QEMU Virt Startup Code
// ============================================================================
.section .text.boot, "ax"
.global _start
_start:
    /* Set up stack pointer using bounds from linker.ld */
    la sp, _estack

    /* Clear BSS section */
    la t0, _sbss
    la t1, _ebss
1:
    bgeu t0, t1, 2f
    sw x0, 0(t0)
    addi t0, t0, 4
    j 1b
2:

    /* Jump to C main entry point */
    call main

    /* 7. Power off QEMU RISC-V Virt via SiFive Test device at 0x100000 */
    /* This prevents QEMU from hanging and spamming log files! */
    li t0, 0x100000
    li t1, 0x5555
    sw t1, 0(t0)

    /* Fallback halt if power-off fails */
3:  wfi
    j 3b


#elif defined(__i386__)
// ============================================================================
// x86 32-bit (i686) Multiboot Startup Code
// ============================================================================
/* Multiboot header only needs to be allocatable ("a") */
.section .multiboot, "a"
.align 4
.long 0x1BADB002                  /* Multiboot magic number */
.long 0x00                        /* Flags */
.long -(0x1BADB002 + 0x00)        /* Checksum */

/* Added "ax" so the linker doesn't strip the x86 startup code */
.section .text.boot, "ax"
.global _start
_start:
    /* Set up stack pointer */
    mov $_estack, %esp

    /* Clear BSS section */
    mov $_sbss, %edi
    mov $_ebss, %ecx
    sub %edi, %ecx
    xor %eax, %eax
    rep stosb

    /* Call C main entry point */
    call main

    /* Halt if main ever returns */
halt:
    hlt
    jmp halt

#endif
