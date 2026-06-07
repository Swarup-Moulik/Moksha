#pragma once
#include <stddef.h>

// Implemented by sys/*/alloc.c or baremetal/core/alloc.c
void *sys_alloc(size_t size);
void *sys_realloc(void *ptr, size_t new_size);
void sys_free(void *ptr);
