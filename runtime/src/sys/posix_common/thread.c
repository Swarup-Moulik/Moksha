#include "../../abi/sys_thread.h"
#include <pthread.h>
#include <stdint.h>

sys_err_t sys_thread_create(sys_thread_t *thread, sys_thread_func_t func,
                            void *arg) {
  pthread_t pt;
  // We cast the func to match the expected pthread signature: void* (*)(void*)
  if (pthread_create(&pt, NULL, (void *(*)(void *))(uintptr_t)func, arg) == 0) {
    *thread = (sys_thread_t)pt;
    return SYS_OK;
  }
  return SYS_ERR_UNKNOWN;
}

sys_err_t sys_thread_join(sys_thread_t thread, void **retval) {
  if (pthread_join((pthread_t)thread, retval) == 0) {
    return SYS_OK;
  }
  return SYS_ERR_UNKNOWN;
}

sys_err_t sys_thread_detach(sys_thread_t thread) {
  if (pthread_detach((pthread_t)thread) == 0) {
    return SYS_OK;
  }
  return SYS_ERR_UNKNOWN;
}
