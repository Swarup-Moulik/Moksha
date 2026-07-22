#include "../../include/moksha_rt.h"

/** @brief Define canonical VTables used by the runtime */
const AnyVTable vtable_string = {MOKSHA_TYPE_STRING,
                                 (void *)__moksha_ptr_to_string,
                                 moksha_rt_retain, moksha_rt_release};

const AnyVTable vtable_i32 = {MOKSHA_TYPE_I32, (void *)__moksha_int_to_string,
                              NULL, NULL};

const AnyVTable vtable_f64 = {MOKSHA_TYPE_F64,
                              (void *)__moksha_double_to_string, NULL, NULL};

const AnyVTable vtable_bool = {MOKSHA_TYPE_BOOL,
                               (void *)__moksha_bool_to_string, NULL, NULL};

const AnyVTable vtable_map = {MOKSHA_TYPE_TABLE, (void *)__moksha_any_to_string,
                              moksha_rt_retain, moksha_rt_release};

const AnyVTable vtable_array = {MOKSHA_TYPE_ARRAY,
                                (void *)__moksha_any_to_string,
                                moksha_rt_retain, moksha_rt_release};

MokshaAny *moksha_box_string(char *str) {
  MokshaAny *any = moksha_rt_alloc(sizeof(MokshaAny), MOKSHA_TYPE_STRING);
  any->data = str;
  any->vtable = &vtable_string;
  return any;
}

MokshaAny *moksha_box_i32(int32_t val) {
  MokshaAny *any = moksha_rt_alloc(sizeof(MokshaAny), MOKSHA_TYPE_I32);
  int32_t *heap_val = moksha_rt_alloc(sizeof(int32_t), MOKSHA_TYPE_I32);
  *heap_val = val;
  any->data = heap_val;
  any->vtable = &vtable_i32;
  return any;
}

MokshaAny *moksha_box_f64(double val) {
  MokshaAny *any = moksha_rt_alloc(sizeof(MokshaAny), MOKSHA_TYPE_F64);
  double *heap_val = moksha_rt_alloc(sizeof(double), MOKSHA_TYPE_F64);
  *heap_val = val;
  any->data = heap_val;
  any->vtable = &vtable_f64;
  return any;
}

MokshaAny *moksha_box_bool(bool val) {
  MokshaAny *any = moksha_rt_alloc(sizeof(MokshaAny), MOKSHA_TYPE_BOOL);
  bool *heap_val = moksha_rt_alloc(sizeof(bool), MOKSHA_TYPE_BOOL);
  *heap_val = val;
  any->data = heap_val;
  any->vtable = &vtable_bool;
  return any;
}

MokshaAny *moksha_box_map(void *map_ptr) {
  MokshaAny *any = moksha_rt_alloc(sizeof(MokshaAny), MOKSHA_TYPE_TABLE);
  any->data = map_ptr;
  any->vtable = &vtable_map;
  return any;
}

MokshaAny *moksha_box_array(MokshaSlice *slice) {
  MokshaAny *any = moksha_rt_alloc(sizeof(MokshaAny), MOKSHA_TYPE_ARRAY);
  any->data = slice;
  any->vtable = &vtable_array;
  return any;
}
