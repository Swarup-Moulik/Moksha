#include "../include/moksha_rt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int32_t moksha_rt_string_length(MokshaString *str) {
  if (!str)
    return 0;
  return (int32_t)str->length;
}

char moksha_rt_string_at(MokshaString *str, int32_t index) {
  if (!str || !str->chars)
    moksha_rt_panic("Null pointer exception: Null string", __FILE__, __LINE__);
  if (index < 0 || (uint64_t)index >= str->length)
    moksha_rt_panic("String index out of bounds", __FILE__, __LINE__);
  return str->chars[index];
}

void moksha_rt_print(MokshaAny *any_val) {
  if (!any_val || !any_val->data) {
    printf("null");
    return;
  }

  // Note: These mappings must align with how MLIR packs the AnyType!
  switch (any_val->type_id) {
  case 1:
    printf("%d", *(int32_t *)any_val->data);
    break;
  case 2:
    printf("%f", *(float *)any_val->data);
    break;
  case 3:
    printf("%s", ((MokshaString *)any_val->data)->chars);
    break;
  default:
    printf("<Object:%p>", any_val->data);
    break;
  }
}

void moksha_rt_println(MokshaAny *any_val) {
  moksha_rt_print(any_val);
  printf("\n");
}

MokshaString *moksha_rt_readFile(MokshaAny *file_val) {
  if (!file_val || !file_val->data || file_val->type_id != 3) {
    moksha_rt_panic("readFile requires a string filename", __FILE__, __LINE__);
  }

  const char *filename = ((MokshaString *)file_val->data)->chars;
  FILE *file = fopen(filename, "rb");
  if (!file)
    moksha_rt_panic("Failed to open file", __FILE__, __LINE__);

  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *buffer = (char *)malloc(length + 1);
  if (!buffer)
    moksha_rt_panic("Out of memory reading file", __FILE__, __LINE__);

  fread(buffer, 1, length, file);
  buffer[length] = '\0';
  fclose(file);

  MokshaString *mStr = (MokshaString *)malloc(sizeof(MokshaString));
  mStr->chars = buffer;
  mStr->length = length;
  return mStr;
}

void moksha_rt_close(MokshaAny *file_val) {
  // Implementation depends on how file handles are stored in MokshaAny.
  // For now, this is a clean no-op so the compiler passes lowering.
}

void *moksha_rt_spawn(MokshaClosure closure) {
  moksha_rt_panic("Concurrency not yet initialized on this platform", __FILE__,
                  __LINE__);
  return NULL;
}

void *moksha_rt_await(void *promise_handle) {
  moksha_rt_panic("Concurrency not yet initialized on this platform", __FILE__,
                  __LINE__);
  return NULL;
}
