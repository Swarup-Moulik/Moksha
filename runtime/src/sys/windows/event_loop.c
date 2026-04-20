#include "../../abi/sys_event.h"
#include <windows.h>

static HANDLE iocp_handle = NULL;

sys_err_t sys_event_init(void) {
  iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
  return iocp_handle ? SYS_OK : SYS_ERR_UNKNOWN;
}

int32_t sys_event_poll(int timeout_ms) {
  DWORD bytes;
  ULONG_PTR key;
  LPOVERLAPPED overlapped;

  // Wait for the OS to signal that an async operation (like a file read) is
  // done
  BOOL success = GetQueuedCompletionStatus(iocp_handle, &bytes, &key,
                                           &overlapped, (DWORD)timeout_ms);

  if (success && key) {
    sys_task_waker_t waker = (sys_task_waker_t)key;
    void *ctx = (void *)overlapped; // In Moksha, this is the coro handle
    waker(ctx);
    return 1;
  }
  return 0;
}
