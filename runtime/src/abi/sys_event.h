// runtime/src/abi/sys_event.h
#pragma once
#include "sys_error.h"
#include <stdint.h>

// A callback function type provided by the core scheduler to resume a coroutine
typedef void (*sys_task_waker_t)(void* ctx);

// Initializes the OS event queue (e.g., epoll_create() or CreateIoCompletionPort())
sys_err_t sys_event_init(void);

// Registers an I/O descriptor with the OS.
// When the fd is ready, the OS loop must call `waker(ctx)`.
// Guarantees: The waker is called EXACTLY once per registration (One-Shot / Edge-Triggered).
sys_err_t sys_event_register_io(int32_t fd, sys_task_waker_t waker, void* ctx);

// Registers a timer with the OS.
// When 'timeout_ms' has elapsed, the OS loop must call `waker(ctx)`.
sys_err_t sys_event_register_timer(uint64_t timeout_ms, sys_task_waker_t waker, void* ctx);

// Blocks the current thread until an event fires or 'timeout_ms' is reached.
// - Returns the number of events processed.
// - Returns SYS_ERR_AGAIN if interrupted.
int32_t sys_event_poll(int timeout_ms);
