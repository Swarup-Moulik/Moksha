#pragma once
#include "sys_error.h"

typedef void *sys_thread_t;
typedef void *(*sys_thread_func_t)(void *);

sys_err_t sys_thread_create(sys_thread_t *thread, sys_thread_func_t func,
                            void *arg);
sys_err_t sys_thread_join(sys_thread_t thread, void **retval);
sys_err_t sys_thread_detach(sys_thread_t thread);
