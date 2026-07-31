#include <stddef.h>
#include <stdint.h>

#define EFIAPI __attribute__((ms_abi))

// Globals to satisfy the linker
void *g_EfiSystemTable = NULL;
char _sstack = 0;
char _estack = 0;
int _fltused = 0;

// Link against the actual Moksha compiler entry point
extern void __moksha_main(void);

EFIAPI uint64_t efi_main(void *ImageHandle, void *SystemTable) {
  (void)ImageHandle;
  g_EfiSystemTable = SystemTable;

  // Cast SystemTable to an array of 64-bit pointers
  uint64_t *st = (uint64_t *)SystemTable;
  uint64_t *ConOut = (uint64_t *)st[8];

  uint64_t(EFIAPI * OutputString)(void *This, const uint16_t *String) =
      (uint64_t(EFIAPI *)(void *, const uint16_t *))ConOut[1];
  uint64_t(EFIAPI * ClearScreen)(void *This) =
      (uint64_t(EFIAPI *)(void *))ConOut[5];

  // 1. Wipe the TianoCore logo
  ClearScreen(ConOut);

  // 2. Print success message
  OutputString(
      ConOut,
      (uint16_t *)L"UEFI raw pointers initialized! Booting Moksha OS...\r\n");

  // 3. Hand control to the Moksha compiler's generated code
  __moksha_main();

  // 4. Halt
  OutputString(ConOut, (uint16_t *)L"\r\nKernel halted safely.\r\n");
  while (1) {
    __asm__ volatile("pause");
  }
  return 0;
}
