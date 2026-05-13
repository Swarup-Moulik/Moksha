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

  // Wait for the OS to signal that an async I/O operation is done
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

// ============================================================================
// Asynchronous Timers (Threadpool API)
// ============================================================================

typedef struct {
  sys_task_waker_t waker;
  void *ctx;
} TimerCtx;

// This callback is executed by a Windows OS threadpool worker when the timer
// expires
static VOID CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE Instance,
                                   PVOID Context, PTP_TIMER Timer) {
  TimerCtx *tctx = (TimerCtx *)Context;

  // Trigger the Moksha waker (resolves the promise & schedules the coroutine)
  tctx->waker(tctx->ctx);

  // Clean up memory and the timer object
  HeapFree(GetProcessHeap(), 0, tctx);
  CloseThreadpoolTimer(Timer);
}

sys_err_t sys_event_register_timer(uint64_t timeout_ms, sys_task_waker_t waker,
                                   void *ctx) {
  TimerCtx *tctx = (TimerCtx *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                         sizeof(TimerCtx));
  if (!tctx)
    return SYS_ERR_NOMEM;

  tctx->waker = waker;
  tctx->ctx = ctx;

  PTP_TIMER timer = CreateThreadpoolTimer(TimerCallback, tctx, NULL);
  if (!timer) {
    HeapFree(GetProcessHeap(), 0, tctx);
    return SYS_ERR_UNKNOWN;
  }

  // Windows timers use 100-nanosecond intervals.
  // A negative value indicates relative time from "now".
  ULARGE_INTEGER ul;
  ul.QuadPart = (ULONGLONG)timeout_ms * -10000ULL;

  FILETIME ft;
  ft.dwHighDateTime = ul.HighPart;
  ft.dwLowDateTime = ul.LowPart;

  // Set the timer to fire exactly once
  SetThreadpoolTimer(timer, &ft, 0, 0);

  return SYS_OK;
}
