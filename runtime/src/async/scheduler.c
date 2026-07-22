#include "../abi/sys_event.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern void *moksha_mem_alloc(size_t size);
extern void moksha_mem_free(void *ptr);
extern void moksha_rt_panic(const char *message);
extern void moksha_trace_coro_state(void *handle, const char *state);
extern void moksha_rt_resume_coro(void *handle);

bool moksha_is_background_task = false;

void moksha_trace_coro_state(void *handle, const char *state) {
  // Tracing disabled/handled by external sys_io if needed
}

/** @brief Priority Registry */

typedef struct PriorityNode {
  void *coro_handle;
  int32_t priority;
  struct PriorityNode *next;
} PriorityNode;

static PriorityNode *priority_registry = NULL;

// A simple linked-list queue for cooperative tasks
typedef struct TaskNode {
  void *coro_handle;
  int32_t priority;
  struct TaskNode *next;
} TaskNode;

static TaskNode *queue_head = NULL;
static TaskNode *queue_tail = NULL;
static uint32_t active_tasks = 0;
static int sched_lock = 0;

static inline void acquire_sched_lock(void) {
  while (__atomic_exchange_n(&sched_lock, 1, __ATOMIC_ACQUIRE)) {
    // Spin lightly
  }
}

static inline void release_sched_lock(void) {
  __atomic_store_n(&sched_lock, 0, __ATOMIC_RELEASE);
}

void moksha_scheduler_set_priority(void *coro_handle, int32_t priority) {
  acquire_sched_lock();
  PriorityNode *curr = priority_registry;
  while (curr) {
    if (curr->coro_handle == coro_handle) {
      curr->priority = priority;
      release_sched_lock();
      return;
    }
    curr = curr->next;
  }
  PriorityNode *node = (PriorityNode *)moksha_mem_alloc(sizeof(PriorityNode));
  node->coro_handle = coro_handle;
  node->priority = priority;
  node->next = priority_registry;
  priority_registry = node;
  release_sched_lock();
}

int32_t moksha_scheduler_get_priority(void *coro_handle) {
  int32_t p = 1; // Default NORMAL
  acquire_sched_lock();
  PriorityNode *curr = priority_registry;
  while (curr) {
    if (curr->coro_handle == coro_handle) {
      p = curr->priority;
      break;
    }
    curr = curr->next;
  }
  release_sched_lock();
  return p;
}

void moksha_scheduler_remove_priority(void *coro_handle) {
  acquire_sched_lock();
  PriorityNode *curr = priority_registry;
  PriorityNode *prev = NULL;
  while (curr) {
    if (curr->coro_handle == coro_handle) {
      if (prev) {
        prev->next = curr->next;
      } else {
        priority_registry = curr->next;
      }
      moksha_mem_free(curr);
      release_sched_lock();
      return;
    }
    prev = curr;
    curr = curr->next;
  }
  release_sched_lock();
}

void moksha_scheduler_init(void) {
  queue_head = NULL;
  queue_tail = NULL;
  priority_registry = NULL;
  active_tasks = 0;
  sched_lock = 0;
}

// Enqueues a coroutine handle to be resumed (Priority Sorted)
void moksha_scheduler_schedule(void *coro_handle) {
  if (!coro_handle)
    return;

  int32_t priority = moksha_scheduler_get_priority(coro_handle);

  TaskNode *node = (TaskNode *)moksha_mem_alloc(sizeof(TaskNode));
  node->coro_handle = coro_handle;
  node->priority = priority;
  node->next = NULL;

  acquire_sched_lock(); // <-- LOCK

  // Sorted insert (Higher priority numbers are placed closer to the head)
  if (!queue_head || queue_head->priority < priority) {
    node->next = queue_head;
    queue_head = node;
    if (!queue_tail) {
      queue_tail = node;
    }
  } else {
    TaskNode *curr = queue_head;
    while (curr->next && curr->next->priority >= priority) {
      curr = curr->next;
    }
    node->next = curr->next;
    curr->next = node;
    if (!node->next) {
      queue_tail = node;
    }
  }

  release_sched_lock(); // <-- UNLOCK

  moksha_trace_coro_state(coro_handle, "Scheduled");
}

void moksha_scheduler_run(void) {
  acquire_sched_lock();

  // If there are absolutely no tasks left, bail out safely.
  if (active_tasks == 0 && !queue_head) {
    release_sched_lock();
    return;
  }

  if (queue_head) {
    TaskNode *node = queue_head;
    queue_head = node->next;
    if (!queue_head) {
      queue_tail = NULL;
    }

    // AGING LOGIC (Starvation Prevention)
    // Boost the priority of all waiting tasks so LOW tasks eventually run.
    TaskNode *curr = queue_head;
    while (curr) {
      curr->priority++;
      curr = curr->next;
    }

    release_sched_lock();

    // Execute the coroutine slice
    moksha_trace_coro_state(node->coro_handle, "Resuming");
    moksha_rt_resume_coro(node->coro_handle);
    moksha_mem_free(node);

  } else {
    release_sched_lock();

    if (active_tasks > 0) {
      sys_event_poll(10);
    }
  }
}

void moksha_scheduler_inc_active(void) {
  acquire_sched_lock();
  active_tasks++;
  release_sched_lock();
}

void moksha_scheduler_dec_active(void) {
  acquire_sched_lock();
  if (active_tasks > 0)
    active_tasks--;
  release_sched_lock();
}

bool moksha_scheduler_is_active(void) {
  acquire_sched_lock();
  bool is_active = (active_tasks > 0 || queue_head != NULL);
  release_sched_lock();

  return is_active;
}
