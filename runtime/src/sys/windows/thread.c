#include "../../abi/sys_thread.h"
#include <windows.h>

sys_err_t sys_thread_create(sys_thread_t *thread, sys_thread_func_t func,
                            void *arg) {
  HANDLE hThread = CreateThread(
      NULL, 0, (LPTHREAD_START_ROUTINE)(uintptr_t)func, arg, 0, NULL);
  if (hThread == NULL)
    return SYS_ERR_UNKNOWN;

  *thread = (sys_thread_t)hThread;
  return SYS_OK;
}

sys_err_t sys_thread_join(sys_thread_t thread, void **retval) {
  WaitForSingleObject((HANDLE)thread, INFINITE);
  CloseHandle((HANDLE)thread);
  return SYS_OK;
}

sys_err_t sys_thread_detach(sys_thread_t thread) {
  if (thread) {
    CloseHandle((HANDLE)thread);
    return SYS_OK;
  }
  return SYS_ERR_UNKNOWN;
}
