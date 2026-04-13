#include "../include/moksha_rt.h"
#include <stdlib.h>

void moksha_rt_retain(void *ptr) {
  if (!ptr)
    return;
  MokshaHeader *obj = (MokshaHeader *)ptr;
  __atomic_fetch_add(&obj->ref_count, 1, __ATOMIC_SEQ_CST);
}

void moksha_rt_release(void *ptr) {
  if (!ptr)
    return;
  MokshaHeader *obj = (MokshaHeader *)ptr;
  uint32_t prev_count =
      __atomic_fetch_sub(&obj->ref_count, 1, __ATOMIC_SEQ_CST);

  if (prev_count == 0) {
    moksha_rt_panic("Fatal: Double free or corrupted ARC state detected!",
                    __FILE__, __LINE__);
  }

  if (prev_count == 1) {
    free(ptr);
  }
}
