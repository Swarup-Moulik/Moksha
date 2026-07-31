#include <stdint.h>

extern void main(void);
extern char _sstack, _estack;
extern char _sdata, _edata, _sidata;
extern char _sbss, _ebss;

void Reset_Handler(void) {
  // Copy .data to RAM
  char *src = &_sidata;
  char *dst = &_sdata;
  while (dst < &_edata)
    *dst++ = *src++;

  // Zero .bss
  dst = &_sbss;
  while (dst < &_ebss)
    *dst++ = 0;

  // Run the kernel
  main();

  // ==========================================================================
  // Auto-Shutdown QEMU via ARM Semihosting
  // ==========================================================================
  register uint32_t r0 __asm__("r0") = 0x18; // 0x18 is Semihosting SYS_EXIT
  register uint32_t r1 __asm__("r1") =
      0x20026; // ADP_Stopped_ApplicationExit parameter

  __asm__ volatile("bkpt 0xAB\n" : : "r"(r0), "r"(r1) : "memory");

  // Fallback halt if semihosting is not enabled
  while (1)
    ;
}

// Minimal vector table
__attribute__((section(".isr_vector"))) void (*const g_pfnVectors[])(void) = {
    (void (*)(void)) & _estack,
    Reset_Handler,
    0, // NMI
    0, // HardFault
    0, // MemManage
    0, // BusFault
    0, // UsageFault
};
