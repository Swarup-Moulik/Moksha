#include "abi/sys_alloc.h"
#include <stddef.h>
#include <stdint.h>

#define EFIAPI __attribute__((ms_abi))
#define EfiLoaderData 2

typedef struct EFI_BOOT_SERVICES {
  char _pad[64]; // AllocatePool is exactly at offset 64
  uint64_t(EFIAPI *AllocatePool)(uint32_t PoolType, size_t Size, void **Buffer);
  uint64_t(EFIAPI *FreePool)(void *Buffer);
} EFI_BOOT_SERVICES;

typedef struct {
  char _pad[96]; // BootServices is exactly at offset 96
  EFI_BOOT_SERVICES *BootServices;
} EFI_SYSTEM_TABLE;

extern void *g_EfiSystemTable;

void *sys_alloc(size_t size) {
  if (!g_EfiSystemTable)
    return NULL;
  EFI_SYSTEM_TABLE *st = (EFI_SYSTEM_TABLE *)g_EfiSystemTable;

  void *buffer = NULL;
  st->BootServices->AllocatePool(EfiLoaderData, size, &buffer);

  if (buffer) {
    char *p = (char *)buffer;
    for (size_t i = 0; i < size; i++)
      p[i] = 0;
  }
  return buffer;
}

void *sys_realloc(void *ptr, size_t new_size) {
  if (!ptr)
    return sys_alloc(new_size);
  return sys_alloc(new_size);
}

void sys_free(void *ptr) {
  if (!g_EfiSystemTable || !ptr)
    return;
  EFI_SYSTEM_TABLE *st = (EFI_SYSTEM_TABLE *)g_EfiSystemTable;
  st->BootServices->FreePool(ptr);
}
