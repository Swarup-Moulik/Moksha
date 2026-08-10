#include <stddef.h>
#include <stdint.h>

#define EFIAPI __attribute__((ms_abi))

void *memset(void *s, int c, size_t n) {
  unsigned char *p = (unsigned char *)s;
  while (n--)
    *p++ = (unsigned char)c;
  return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
  char *d = (char *)dest;
  const char *s = (const char *)src;
  while (n--)
    *d++ = *s++;
  return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
  char *d = (char *)dest;
  const char *s = (const char *)src;
  if (d < s) {
    while (n--)
      *d++ = *s++;
  } else {
    char *lasts = (char *)s + (n - 1);
    char *lastd = d + (n - 1);
    while (n--)
      *lastd-- = *lasts--;
  }
  return dest;
}

void *g_EfiSystemTable = NULL;
char _sstack = 0;
char _estack = 0;
int _fltused = 0;

uint64_t g_GopBaseAddress = 0;
uint32_t g_GopWidth = 0;
uint32_t g_GopHeight = 0;
uint32_t g_GopPixelsPerScanline = 0;

typedef struct {
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t Data4[8];
} EFI_GUID;

static EFI_GUID gop_guid = {0x9042a9de,
                            0x23dc,
                            0x4a38,
                            {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}};

typedef struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION {
  uint32_t Version;
  uint32_t HorizontalResolution;
  uint32_t VerticalResolution;
  uint32_t PixelFormat;
  uint32_t PixelInformation[4];
  uint32_t PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
  uint32_t MaxMode;
  uint32_t Mode;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
  size_t SizeOfInfo;
  uint64_t FrameBufferBase;
  size_t FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
  char _pad[24];
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

extern void __moksha_module_init(void);
extern void __moksha_main(void);

EFIAPI uint64_t efi_main(void *ImageHandle, void *SystemTable) {
  g_EfiSystemTable = SystemTable;

  uint64_t *st = (uint64_t *)SystemTable;
  uint64_t *ConOut = (uint64_t *)st[8];
  uint64_t *BootServices = (uint64_t *)st[12];

  uint64_t(EFIAPI * OutputString)(void *, const uint16_t *) =
      (uint64_t(EFIAPI *)(void *, const uint16_t *))ConOut[1];
  uint64_t(EFIAPI * ClearScreen)(void *) =
      (uint64_t(EFIAPI *)(void *))ConOut[6];
  uint64_t(EFIAPI * LocateProtocol)(EFI_GUID *, void *, void **) =
      (uint64_t(EFIAPI *)(EFI_GUID *, void *, void **))BootServices[40];

  // We need 3 new UEFI functions to kill the firmware
  uint64_t(EFIAPI * GetMemoryMap)(size_t *, void *, size_t *, size_t *,
                                  uint32_t *) =
      (uint64_t(EFIAPI *)(size_t *, void *, size_t *, size_t *,
                          uint32_t *))BootServices[7];
  uint64_t(EFIAPI * AllocatePool)(uint32_t, size_t, void **) =
      (uint64_t(EFIAPI *)(uint32_t, size_t, void **))BootServices[8];
  uint64_t(EFIAPI * ExitBootServices)(void *, size_t) =
      (uint64_t(EFIAPI *)(void *, size_t))BootServices[29];

  ClearScreen(ConOut);
  OutputString(
      ConOut,
      (uint16_t *)L"Fetching hardware map. Preparing to terminate UEFI...\r\n");

  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
  if (LocateProtocol(&gop_guid, NULL, (void **)&gop) == 0 && gop && gop->Mode) {
    g_GopBaseAddress = gop->Mode->FrameBufferBase;
    g_GopWidth = gop->Mode->Info->HorizontalResolution;
    g_GopHeight = gop->Mode->Info->VerticalResolution;
    g_GopPixelsPerScanline = gop->Mode->Info->PixelsPerScanLine;
  }

  // 1. Init Moksha globals BEFORE we kill UEFI so FONT_8X16 gets populated
  __moksha_module_init();

  // 2. Loop to successfully terminate UEFI
  size_t MapSize = 0, MapKey = 0, DescriptorSize = 0;
  uint32_t DescriptorVersion = 0;
  void *MemoryMap = NULL;

  // First call to get the required size for the memory map buffer
  GetMemoryMap(&MapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);

  while (1) {
    // Add extra padding space because AllocatePool alters the memory map table
    MapSize += DescriptorSize * 2;
    AllocatePool(2, MapSize, &MemoryMap); // 2 = EfiLoaderData

    // Fetch the actual map and the fresh MapKey
    if (GetMemoryMap(&MapSize, MemoryMap, &MapKey, &DescriptorSize,
                     &DescriptorVersion) == 0) {
      if (ExitBootServices(ImageHandle, MapKey) == 0) {
        break;
      }
    }
  }

  // Declare the Moksha function from pmm.mox
  extern void pmm_init(void *memory_map, uint64_t map_size, uint64_t desc_size);

  // Hand the salvaged hardware map directly to Moksha!
  pmm_init(MemoryMap, MapSize, DescriptorSize);

  // Boot the OS
  __moksha_main();

  while (1) {
    __asm__ volatile("cli; hlt");
  }
  return 0;
}
