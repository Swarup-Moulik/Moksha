#include "../abi/sys_event.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

extern void *moksha_mem_alloc(size_t size);
extern void moksha_mem_free(void *ptr);
extern void moksha_rt_panic(const char *message);
extern void moksha_trace_coro_state(void *handle, const char *state);
extern void moksha_rt_resume_coro(void *handle);

void moksha_trace_coro_state(void *handle, const char *state) {
  // printf("[SCHEDULER] Coroutine %p -> %s\n", handle, state);
  // fflush(stdout);
}

// A simple linked-list queue for cooperative tasks
typedef struct TaskNode {
  void *coro_handle;
  struct TaskNode *next;
} TaskNode;

static TaskNode *queue_head = NULL;
static TaskNode *queue_tail = NULL;

// Tracks ALL active tasks (running, waiting in queue, or suspended awaiting
// I/O)
static uint32_t active_tasks = 0;

void moksha_scheduler_init(void) {
  queue_head = NULL;
  queue_tail = NULL;
  active_tasks = 0;
}

// Enqueues a coroutine handle to be resumed
void moksha_scheduler_schedule(void *coro_handle) {
  if (!coro_handle)
    return;

  TaskNode *node = (TaskNode *)moksha_mem_alloc(sizeof(TaskNode));
  node->coro_handle = coro_handle;
  node->next = NULL;

  if (!queue_head) {
    queue_head = node;
    queue_tail = node;
  } else {
    queue_tail->next = node;
    queue_tail = node;
  }

  active_tasks++; // Increment active tasks when scheduled
  moksha_trace_coro_state(coro_handle, "Scheduled");
}

void moksha_scheduler_run(void) {
  // Spin as long as there is ANY active task, even if the queue is empty
  while (active_tasks > 0) {
    if (queue_head) {
      TaskNode *node = queue_head;
      queue_head = node->next;
      if (!queue_head) {
        queue_tail = NULL;
      }

      // Decrement active_tasks because this task is now resolving
      active_tasks--;

      moksha_trace_coro_state(node->coro_handle, "Resuming");
      moksha_rt_resume_coro(node->coro_handle);

      // Free the node
      moksha_mem_free(node);
    } else {
      // The queue is empty, but active_tasks > 0.
      // This means coroutines are suspended waiting on OS Threads or I/O.
      // Block and let the OS Event Loop wake us up!
      sys_event_poll(100);
    }
  }
}

// API for OS threads to keep the scheduler alive
void moksha_scheduler_inc_active(void) { active_tasks++; }

void moksha_scheduler_dec_active(void) {
  if (active_tasks > 0)
    active_tasks--;
}
