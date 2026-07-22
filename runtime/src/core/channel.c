#include "../../include/moksha_rt.h"
#include <stdbool.h>

typedef struct WaitNode {
  void *promise_handle;
  void *data;
  struct WaitNode *next;
} WaitNode;

typedef struct {
  int spin_lock;
  bool is_closed;
  void **buffer;
  int capacity;
  int head;
  int tail;
  int count;
  WaitNode *waiting_receivers;
  WaitNode *waiting_senders;
} MokshaChannel;

extern void *moksha_rt_alloc(size_t payload_size, uint32_t type_id);
extern void *moksha_mem_alloc(size_t size);
extern void moksha_mem_free(void *ptr);
extern void moksha_rt_panic(const char *message);
extern void *moksha_rt_make_unresolved_promise(void);
extern void moksha_rt_resolve_promise(void *promise_handle, void *result_data);
extern void moksha_rt_reject_promise(void *promise_handle, void *ex_payload);
extern void cpu_relax(void);

void *moksha_builtin_Channel_new(int capacity) {
  void *ptr = moksha_rt_alloc(sizeof(MokshaChannel), MOKSHA_TYPE_CHANNEL);
  MokshaChannel *chan = (MokshaChannel *)ptr;
  chan->spin_lock = 0;
  chan->is_closed = false;
  chan->capacity = capacity;
  chan->head = 0;
  chan->tail = 0;
  chan->count = 0;
  chan->buffer = (void **)moksha_mem_alloc(capacity * sizeof(void *));
  chan->waiting_receivers = NULL;
  chan->waiting_senders = NULL;
  return ptr;
}

void *moksha_builtin_Channel_recv(void *this_ptr) {
  MokshaChannel *chan = (MokshaChannel *)this_ptr;
  void *promise = moksha_rt_make_unresolved_promise();

  while (__atomic_exchange_n(&chan->spin_lock, 1, __ATOMIC_ACQUIRE)) {
    cpu_relax();
  }

  if (chan->count > 0) {
    void *data = chan->buffer[chan->head];
    chan->head = (chan->head + 1) % chan->capacity;
    chan->count--;

    if (chan->waiting_senders) {
      WaitNode *sender = chan->waiting_senders;
      chan->waiting_senders = sender->next;

      chan->buffer[chan->tail] = sender->data;
      chan->tail = (chan->tail + 1) % chan->capacity;
      chan->count++;

      moksha_rt_resolve_promise(sender->promise_handle, NULL);
      moksha_mem_free(sender);
    }

    __atomic_store_n(&chan->spin_lock, 0, __ATOMIC_RELEASE);
    moksha_rt_resolve_promise(promise, data);
  } else if (chan->is_closed) {
    __atomic_store_n(&chan->spin_lock, 0, __ATOMIC_RELEASE);
    moksha_rt_reject_promise(promise, moksha_rt_alloc(8, 22));
  } else {
    WaitNode *node = (WaitNode *)moksha_mem_alloc(sizeof(WaitNode));
    node->promise_handle = promise;
    node->next = chan->waiting_receivers;
    chan->waiting_receivers = node;
    __atomic_store_n(&chan->spin_lock, 0, __ATOMIC_RELEASE);
  }

  return promise;
}

void *moksha_builtin_Channel_send(void *this_ptr, void *val) {
  MokshaChannel *chan = (MokshaChannel *)this_ptr;
  void *promise = moksha_rt_make_unresolved_promise();

  while (__atomic_exchange_n(&chan->spin_lock, 1, __ATOMIC_ACQUIRE)) {
    cpu_relax();
  }

  if (chan->is_closed) {
    __atomic_store_n(&chan->spin_lock, 0, __ATOMIC_RELEASE);
    moksha_rt_reject_promise(promise, moksha_rt_alloc(8, 22));
    return promise;
  }

  if (chan->waiting_receivers) {
    WaitNode *recv = chan->waiting_receivers;
    chan->waiting_receivers = recv->next;

    __atomic_store_n(&chan->spin_lock, 0, __ATOMIC_RELEASE);
    moksha_rt_resolve_promise(recv->promise_handle, val);
    moksha_mem_free(recv);

    moksha_rt_resolve_promise(promise, NULL);
  } else if (chan->count < chan->capacity) {
    chan->buffer[chan->tail] = val;
    chan->tail = (chan->tail + 1) % chan->capacity;
    chan->count++;
    __atomic_store_n(&chan->spin_lock, 0, __ATOMIC_RELEASE);
    moksha_rt_resolve_promise(promise, NULL);
  } else {
    WaitNode *node = (WaitNode *)moksha_mem_alloc(sizeof(WaitNode));
    node->promise_handle = promise;
    node->data = val;
    node->next = chan->waiting_senders;
    chan->waiting_senders = node;
    __atomic_store_n(&chan->spin_lock, 0, __ATOMIC_RELEASE);
  }

  return promise;
}

void moksha_builtin_Channel_close(void *this_ptr) {
  MokshaChannel *chan = (MokshaChannel *)this_ptr;

  while (__atomic_exchange_n(&chan->spin_lock, 1, __ATOMIC_ACQUIRE)) {
    cpu_relax();
  }

  chan->is_closed = true;

  // Flush waiting receivers safely
  while (chan->waiting_receivers) {
    WaitNode *recv = chan->waiting_receivers;
    chan->waiting_receivers = recv->next;

    // Use the safe rejection API
    moksha_rt_reject_promise(recv->promise_handle, moksha_rt_alloc(8, 22));
    moksha_mem_free(recv);
  }

  // Flush waiting senders safely
  while (chan->waiting_senders) {
    WaitNode *sender = chan->waiting_senders;
    chan->waiting_senders = sender->next;

    moksha_rt_reject_promise(sender->promise_handle, moksha_rt_alloc(8, 22));
    moksha_mem_free(sender);
  }

  __atomic_store_n(&chan->spin_lock, 0, __ATOMIC_RELEASE);
}
