#include "../include/moksha_rt.h"
#include <stdio.h>
#include <stdlib.h>

void moksha_rt_panic(const char *message, const char *file, uint32_t line) {
  fprintf(stderr, "\n========================================\n");
  fprintf(stderr, "MOKSHA RUNTIME PANIC\n");
  fprintf(stderr, "========================================\n");
  fprintf(stderr, "Error: %s\n", message);
  fprintf(stderr, "Location: %s:%u\n", file, line);
  fprintf(stderr, "========================================\n");
  abort();
}
