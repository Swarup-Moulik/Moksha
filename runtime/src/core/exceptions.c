#include "../../include/moksha_rt.h"
#include <unwind.h>

extern void *moksha_mem_alloc(size_t size);
extern void moksha_mem_free(void *ptr);
extern void moksha_rt_panic(const char *message);
extern __thread void *__moksha_ex_payload;

struct MokshaException {
  struct _Unwind_Exception base;
  void *payload;
};

static void cleanup_exception(_Unwind_Reason_Code reason,
                              struct _Unwind_Exception *exc) {
  moksha_mem_free(exc);
}

void *moksha_rt_consume_exception() {
  void *ex = __moksha_ex_payload;
  __moksha_ex_payload = NULL;
  return ex;
}

void moksha_rt_throw(void *payload) {
  __moksha_ex_payload = payload;

  struct MokshaException *exc = (struct MokshaException *)moksha_mem_alloc(
      sizeof(struct MokshaException));

  exc->base.exception_class = 0x4D4F4B5348410000ULL;
  exc->base.exception_cleanup = cleanup_exception;
  exc->payload = payload;

  _Unwind_RaiseException(&exc->base);

  moksha_rt_panic("Unhandled Exception: Unwinder failed to find a handler");
}

void *moksha_rt_get_exception_payload(struct _Unwind_Exception *exc_base) {
  if (!exc_base)
    return NULL;
  struct MokshaException *exc = (struct MokshaException *)exc_base;
  return exc->payload;
}

void Exception_constructor_string_ret_void(void *this_ptr, char *msg_str) {
  void **fields = (void **)this_ptr;

  if (msg_str) {
    moksha_rt_retain(msg_str);
  }

  fields[0] = msg_str;
}
