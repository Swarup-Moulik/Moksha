#include "../../include/moksha_rt.h"
#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef _WIN32
#include <io.h>
#define fsync _commit
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

extern void *moksha_rt_alloc(size_t payload_size, uint32_t type_id);
extern void moksha_rt_release(void *ptr);
extern void moksha_mem_free(void *ptr);

// External map & string bindings needed for structured data
extern int32_t moksha_rt_map_len(void *map);
extern MokshaAny *moksha_rt_map_get_key_at(void *map, int32_t index);
extern MokshaAny *moksha_rt_map_get_val_at(void *map, int32_t index);
extern char *__moksha_any_to_string(MokshaAny *any_val);
extern void *moksha_rt_map_new();
extern void moksha_rt_map_insert(void *map, MokshaAny *key, MokshaAny *val);
extern const AnyVTable vtable_map;
extern const AnyVTable vtable_string;
extern const AnyVTable vtable_array;

// ============================================================================
// Internal Utilities
// ============================================================================

static char *make_mstring(const char *cstr, size_t len) {
  char *str = (char *)moksha_rt_alloc(len + 1, 1 /* MOKSHA_TYPE_I8 */);
  memcpy(str, cstr, len);
  str[len] = '\0';
  return str;
}

static inline int unbox_fd(MokshaAny *file_any) {
  if (!file_any || !file_any->data)
    return -1;
  return (int)*(intptr_t *)file_any->data;
}

int32_t length(void *arr_ptr) {
  if (!arr_ptr)
    return 0;
  MokshaAny *any_val = (MokshaAny *)arr_ptr;
  if (!any_val->data)
    return 0;

  // Single unbox ABI matching the compiler
  MokshaSlice *slice = (MokshaSlice *)any_val->data;
  return (int32_t)slice->length;
}

int32_t moksha_rt_any_len(MokshaAny *any_val) {
  if (!any_val || !any_val->vtable || !any_val->data)
    return 0;

  if (any_val->vtable->type_id == MOKSHA_TYPE_STRING) {
    return strlen((char *)any_val->data);
  } else if (any_val->vtable->type_id == MOKSHA_TYPE_ARRAY) {
    // Single unbox ABI matching the compiler
    MokshaSlice *slice = (MokshaSlice *)any_val->data;
    return (int32_t)slice->length;
  } else if (any_val->vtable->type_id == MOKSHA_TYPE_TABLE) {
    return moksha_rt_map_len(any_val->data);
  }
  return 0;
}

// ============================================================================
// Internal Literal Parser for Structured Data
// ============================================================================
static MokshaAny *parse_and_box_literal(const char *val_start, int val_len,
                                        bool is_string) {
  if (is_string) {
    char *val_str = moksha_rt_alloc(val_len + 1, MOKSHA_TYPE_STRING);
    memcpy(val_str, val_start, val_len);
    val_str[val_len] = '\0';
    return moksha_box_string(val_str);
  }

  while (val_len > 0 &&
         (val_start[val_len - 1] == ' ' || val_start[val_len - 1] == '\r')) {
    val_len--;
  }

  char tmp[128];
  int copy_len = val_len < 127 ? val_len : 127;
  memcpy(tmp, val_start, copy_len);
  tmp[copy_len] = '\0';

  if (strncmp(tmp, "true", 4) == 0)
    return moksha_box_bool(true);
  if (strncmp(tmp, "false", 5) == 0)
    return moksha_box_bool(false);
  if (strncmp(tmp, "null", 4) == 0)
    return moksha_box_string(make_mstring("", 0));

  bool is_float = false;
  for (int i = 0; i < copy_len; i++) {
    if (tmp[i] == '.') {
      is_float = true;
      break;
    }
  }

  if (is_float) {
    char *endptr;
    double val = strtod(tmp, &endptr);
    if (endptr != tmp)
      return moksha_box_f64(val);
  } else {
    char *endptr;
    long val = strtol(tmp, &endptr, 10);
    if (endptr != tmp)
      return moksha_box_i32((int32_t)val);
  }

  return parse_and_box_literal(val_start, val_len, true);
}

// ============================================================================
// Raw File Descriptor Builtins
// ============================================================================

MokshaAnyRet moksha_file_open(char *path, int32_t mode) {
  if (!path)
    return moksha_pack_any(NULL, NULL);

  int flags = 0;
  if ((mode & 1) && (mode & 2))
    flags = O_RDWR;
  else if (mode & 2)
    flags = O_WRONLY;
  else
    flags = O_RDONLY;

  if (mode & 4)
    flags |= O_APPEND;
  if (mode & 8)
    flags |= O_BINARY;
  if (mode & 16)
    flags |= O_CREAT;
  if (mode & 32)
    flags |= O_TRUNC;

  int fd = open(path, flags, 0666);
  if (fd == -1)
    return moksha_pack_any(NULL, NULL);

  intptr_t *fd_box = (intptr_t *)moksha_rt_alloc(sizeof(intptr_t), 19);
  *fd_box = fd;

  return moksha_pack_any(fd_box, NULL);
}

void moksha_file_close(MokshaAny *file_any) {
  int fd = unbox_fd(file_any);
  if (fd >= 0)
    close(fd);
}

void moksha_file_write(MokshaAny *file_any, MokshaAny *data_any) {
  int fd = unbox_fd(file_any);
  if (fd < 0 || !data_any || !data_any->data)
    return;
  char *data = (char *)data_any->data;
  write(fd, data, strlen(data));
}

MokshaAnyRet moksha_file_read(MokshaAny *file_any) {
  int fd = unbox_fd(file_any);
  if (fd < 0)
    return moksha_pack_any(NULL, NULL);

  struct stat st;
  if (fstat(fd, &st) < 0)
    return moksha_pack_any(NULL, NULL);

  size_t size = st.st_size;
  char *buf = (char *)moksha_rt_alloc(size + 1, 1);

  ssize_t bytes_read = read(fd, buf, size);
  if (bytes_read < 0)
    bytes_read = 0;
  buf[bytes_read] = '\0';

  // Attach string vtable so structural equality works
  return moksha_pack_any(buf, &vtable_string);
}

int64_t moksha_file_size(MokshaAny *file_any) {
  int fd = unbox_fd(file_any);
  if (fd < 0)
    return 0;
  struct stat st;
  if (fstat(fd, &st) == 0)
    return (int64_t)st.st_size;
  return 0;
}

void moksha_file_seek(MokshaAny *file_any, int64_t pos) {
  int fd = unbox_fd(file_any);
  if (fd >= 0)
    lseek(fd, (off_t)pos, SEEK_SET);
}

int64_t moksha_file_tell(MokshaAny *file_any) {
  int fd = unbox_fd(file_any);
  if (fd < 0)
    return -1;
  return (int64_t)lseek(fd, 0, SEEK_CUR);
}

void moksha_file_flush(MokshaAny *file_any) {
  int fd = unbox_fd(file_any);
  if (fd >= 0)
    fsync(fd);
}

bool moksha_file_eof(MokshaAny *file_any) {
  int fd = unbox_fd(file_any);
  if (fd < 0)
    return true;
  off_t current = lseek(fd, 0, SEEK_CUR);
  struct stat st;
  fstat(fd, &st);
  return current >= st.st_size;
}

bool moksha_file_exists(char *path) {
  if (!path)
    return false;
#ifdef _WIN32
  // Windows _access: 0 means it exists
  return _access(path, 0) == 0;
#else
  // POSIX access: F_OK means file exists
  return access(path, F_OK) == 0;
#endif
}

void moksha_file_truncate(MokshaAny *file_any, int64_t size) {
  int fd = unbox_fd(file_any);
  if (fd >= 0)
    ftruncate(fd, (off_t)size);
}

// ============================================================================
// High-Level Stream IO
// ============================================================================

void moksha_file_writeLine(MokshaAny *file_any, char *text) {
  int fd = unbox_fd(file_any);
  if (fd < 0 || !text)
    return;
  write(fd, text, strlen(text));
  write(fd, "\n", 1);
}

char *moksha_file_readLine(MokshaAny *file_any) {
  int fd = unbox_fd(file_any);
  if (fd < 0)
    return NULL;

  size_t cap = 128;
  size_t len = 0;
  char *buf = (char *)moksha_rt_alloc(cap, 1);
  char c;
  bool read_any = false;

  while (read(fd, &c, 1) == 1) {
    read_any = true;
    if (c == '\n')
      break;
    if (c == '\r')
      continue;

    if (len + 1 >= cap) {
      cap *= 2;
      char *new_buf = (char *)moksha_rt_alloc(cap, 1);
      memcpy(new_buf, buf, len);
      buf = new_buf;
    }
    buf[len++] = c;
  }

  if (!read_any)
    return NULL;
  buf[len] = '\0';
  return buf;
}

MokshaAnyRet moksha_file_readLines(MokshaAny *file_or_path_any) {
  if (!file_or_path_any)
    return moksha_pack_any(NULL, NULL);

  int fd = -1;
  bool should_close = false;

  if (file_or_path_any->vtable && file_or_path_any->vtable->type_id == 16) {
    char *path = (char *)file_or_path_any->data;
    MokshaAnyRet open_ret = moksha_file_open(path, 0);
    MokshaAny temp = {(void *)(uintptr_t)open_ret,
                      (AnyVTable *)(uintptr_t)(open_ret >> 64)};
    fd = unbox_fd(&temp);
    should_close = true;
  } else {
    fd = unbox_fd(file_or_path_any);
  }

  if (fd < 0)
    return moksha_pack_any(NULL, NULL);

  intptr_t fd_box = fd;
  MokshaAny temp_any = {&fd_box, NULL};

  size_t cap = 16;
  size_t count = 0;
  char **arr = (char **)moksha_rt_alloc(cap * sizeof(char *), 2);

  while (true) {
    char *line = moksha_file_readLine(&temp_any);
    if (!line)
      break;

    if (count >= cap) {
      cap *= 2;
      char **new_arr = (char **)moksha_rt_alloc(cap * sizeof(char *), 2);
      memcpy(new_arr, arr, count * sizeof(char *));
      arr = new_arr;
    }
    arr[count++] = line;
  }

  if (should_close)
    close(fd);

  MokshaSlice *slice = (MokshaSlice *)moksha_rt_alloc(sizeof(MokshaSlice), 18);
  slice->data = arr;
  slice->length = count;
  return moksha_pack_any(slice, &vtable_array);
}

char *moksha_file_readText(char *path) {
  if (!path)
    return NULL;

  MokshaAnyRet ret = moksha_file_open(path, 0);
  MokshaAny file_any = {(void *)(uintptr_t)ret,
                        (const AnyVTable *)(uintptr_t)(ret >> 64)};

  if (unbox_fd(&file_any) < 0)
    return NULL;

  MokshaAnyRet result_ret = moksha_file_read(&file_any);
  moksha_file_close(&file_any);

  return (char *)(uintptr_t)result_ret;
}

void moksha_file_writeText(char *path, char *text) {
  if (!path || !text)
    return;
  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd >= 0) {
    write(fd, text, strlen(text));
    close(fd);
  }
}

void moksha_file_appendText(char *path, char *text) {
  if (!path || !text)
    return;
  int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0666);
  if (fd >= 0) {
    write(fd, text, strlen(text));
    close(fd);
  }
}

// ============================================================================
// High-Level Binary IO
// ============================================================================

void moksha_file_writeBytes(char *path, MokshaAny *data_any) {
  if (!path || !data_any || !data_any->data)
    return;

  // Fix: Single unbox to match the compiler's anycast ABI
  MokshaSlice *slice = (MokshaSlice *)data_any->data;

  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
  if (fd >= 0) {
    uint8_t *bytes = malloc(slice->length);
    int32_t *ints = (int32_t *)slice->data;
    for (uint64_t i = 0; i < slice->length; i++) {
      bytes[i] = (uint8_t)ints[i];
    }
    write(fd, bytes, slice->length);
    free(bytes);
    close(fd);
  }
}

void moksha_file_appendBytes(char *path, MokshaAny *data_any) {
  if (!path || !data_any || !data_any->data)
    return;

  // Fix: Single unbox to match the compiler's anycast ABI
  MokshaSlice *slice = (MokshaSlice *)data_any->data;

  int fd = open(path, O_WRONLY | O_CREAT | O_APPEND | O_BINARY, 0666);
  if (fd >= 0) {
    uint8_t *bytes = malloc(slice->length);
    int32_t *ints = (int32_t *)slice->data;
    for (uint64_t i = 0; i < slice->length; i++) {
      bytes[i] = (uint8_t)ints[i];
    }
    write(fd, bytes, slice->length);
    free(bytes);
    close(fd);
  }
}

MokshaAnyRet moksha_file_readBytes(char *path) {
  if (!path)
    return moksha_pack_any(NULL, NULL);

  int fd = open(path, O_RDONLY | O_BINARY);
  if (fd < 0)
    return moksha_pack_any(NULL, NULL);

  struct stat st;
  if (fstat(fd, &st) < 0) {
    close(fd);
    return moksha_pack_any(NULL, NULL);
  }

  size_t size = st.st_size;
  char *buf = (char *)moksha_rt_alloc(size, 1);

  ssize_t bytes_read = read(fd, buf, size);
  if (bytes_read < 0)
    bytes_read = 0;
  close(fd);

  MokshaSlice *slice = (MokshaSlice *)moksha_rt_alloc(sizeof(MokshaSlice), 18);
  slice->data = buf;
  slice->length = bytes_read;
  return moksha_pack_any(slice, &vtable_array);
}

// ============================================================================
// Structured Data (JSON / YAML / PDF File Streamers)
// ============================================================================

void moksha_file_writeJson(char *path, MokshaAny *data_any) {
  void *map_ptr = data_any ? data_any->data : NULL;
  if (!map_ptr)
    return;

  int len = moksha_rt_map_len(map_ptr);

  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
  if (fd < 0)
    return;

  write(fd, "{\n", 2);
  for (int i = 0; i < len; i++) {
    MokshaAny *k = moksha_rt_map_get_key_at(map_ptr, i);
    MokshaAny *v = moksha_rt_map_get_val_at(map_ptr, i);

    char *k_str = (k && k->data) ? (char *)k->data : (char *)"";
    bool is_str = (v && v->vtable && v->vtable->type_id == 16);
    char *v_str = is_str ? (char *)v->data : __moksha_any_to_string(v);

    write(fd, "  \"", 3);
    write(fd, k_str, strlen(k_str));
    write(fd, "\": ", 3);

    if (is_str)
      write(fd, "\"", 1);
    write(fd, v_str, strlen(v_str));
    if (is_str)
      write(fd, "\"", 1);

    if (i < len - 1)
      write(fd, ",", 1);
    write(fd, "\n", 1);
  }
  write(fd, "}\n", 2);
  close(fd);
}

MokshaAnyRet moksha_file_readJson(char *path) {
  char *text = moksha_file_readText(path);
  void *map = moksha_rt_map_new();
  if (!text)
    return moksha_pack_any(map, NULL);

  char *p = text;
  while (*p && *p != '{')
    p++;
  if (*p == '{')
    p++;

  while (*p && *p != '}') {
    while (*p == ' ' || *p == '\n' || *p == '\r' || *p == ',')
      p++;
    if (*p == '}')
      break;

    if (*p == '"')
      p++;
    char *key_start = p;
    while (*p && *p != '"')
      p++;
    int key_len = p - key_start;
    if (*p == '"')
      p++;

    while (*p && *p != ':')
      p++;
    if (*p == ':')
      p++;
    while (*p == ' ' || *p == '\n' || *p == '\r')
      p++;

    bool is_string = (*p == '"');
    if (is_string)
      p++;

    char *val_start = p;
    if (is_string) {
      while (*p && *p != '"')
        p++;
    } else {
      while (*p && *p != ',' && *p != '}' && *p != ' ' && *p != '\n' &&
             *p != '\r')
        p++;
    }
    int val_len = p - val_start;
    if (is_string && *p == '"')
      p++;

    char *key_str = moksha_rt_alloc(key_len + 1, MOKSHA_TYPE_STRING);
    memcpy(key_str, key_start, key_len);
    key_str[key_len] = '\0';
    MokshaAny *kp = moksha_box_string(key_str);

    MokshaAny *vp = parse_and_box_literal(val_start, val_len, is_string);
    moksha_rt_map_insert(map, kp, vp);
  }
  return moksha_pack_any(map, &vtable_map);
}

void moksha_file_writeYaml(char *path, MokshaAny *data_any) {
  void *map_ptr = data_any ? data_any->data : NULL;
  if (!map_ptr)
    return;

  int len = moksha_rt_map_len(map_ptr);

  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
  if (fd < 0)
    return;

  for (int i = 0; i < len; i++) {
    MokshaAny *k = moksha_rt_map_get_key_at(map_ptr, i);
    MokshaAny *v = moksha_rt_map_get_val_at(map_ptr, i);

    char *k_str = (k && k->data) ? (char *)k->data : (char *)"";
    bool is_str = (v && v->vtable && v->vtable->type_id == 16);
    char *v_str = is_str ? (char *)v->data : __moksha_any_to_string(v);

    write(fd, k_str, strlen(k_str));
    write(fd, ": ", 2);
    write(fd, v_str, strlen(v_str));
    write(fd, "\n", 1);
  }
  close(fd);
}

MokshaAnyRet moksha_file_readYaml(char *path) {
  char *text = moksha_file_readText(path);
  void *map = moksha_rt_map_new();
  if (!text)
    return moksha_pack_any(map, NULL);

  char *p = text;
  while (*p) {
    while (*p == ' ' || *p == '\n' || *p == '\r')
      p++;
    if (!*p)
      break;

    char *key_start = p;
    while (*p && *p != ':')
      p++;
    int key_len = p - key_start;
    if (*p == ':')
      p++;
    while (*p == ' ')
      p++;

    char *val_start = p;
    while (*p && *p != '\n' && *p != '\r')
      p++;
    int val_len = p - val_start;

    bool is_string = false;
    if (val_len >= 2 && val_start[0] == '"' && val_start[val_len - 1] == '"') {
      is_string = true;
      val_start++;
      val_len -= 2;
    }

    char *key_str = moksha_rt_alloc(key_len + 1, MOKSHA_TYPE_STRING);
    memcpy(key_str, key_start, key_len);
    key_str[key_len] = '\0';
    MokshaAny *kp = moksha_box_string(key_str);

    MokshaAny *vp = parse_and_box_literal(val_start, val_len, is_string);
    moksha_rt_map_insert(map, kp, vp);
  }
  return moksha_pack_any(map, &vtable_map);
}

// ============================================================================
// CSV File Streamers (Array of Tables)
// ============================================================================

void moksha_file_writeCsv(char *path, MokshaAny *data_any) {
  if (!path || !data_any || !data_any->data)
    return;

  // Single unbox ABI matching the compiler
  MokshaSlice *slice = (MokshaSlice *)data_any->data;
  if (slice->length == 0)
    return;

  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
  if (fd < 0)
    return;

  // Assume data_any is an array of maps. Get the first row to extract headers.
  MokshaAny *first_row = &((MokshaAny *)slice->data)[0];
  void *map_ptr = first_row->data;
  if (!map_ptr) {
    close(fd);
    return;
  }

  int cols = moksha_rt_map_len(map_ptr);

  // Write Headers
  for (int i = 0; i < cols; i++) {
    MokshaAny *k = moksha_rt_map_get_key_at(map_ptr, i);
    char *k_str = "";
    if (k && k->vtable && k->vtable->type_id == 16) {
      k_str = (char *)k->data; // Directly unbox C-string
    }

    bool needs_quotes = strchr(k_str, ',') != NULL;
    if (needs_quotes)
      write(fd, "\"", 1);
    write(fd, k_str, strlen(k_str));
    if (needs_quotes)
      write(fd, "\"", 1);

    if (i < cols - 1)
      write(fd, ",", 1);
  }
  write(fd, "\n", 1);

  // Write Rows
  for (uint64_t r = 0; r < slice->length; r++) {
    MokshaAny *row_any = &((MokshaAny *)slice->data)[r];
    void *row_map = row_any->data;
    if (!row_map)
      continue;

    for (int i = 0; i < cols; i++) {
      MokshaAny *v = moksha_rt_map_get_val_at(row_map, i);

      // Match the JSON/YAML implementation for string extraction
      bool is_str = (v && v->vtable && v->vtable->type_id == 16);
      char *v_str =
          is_str ? (char *)v->data : (v ? __moksha_any_to_string(v) : "");

      bool needs_quotes = strchr(v_str, ',') != NULL;
      if (needs_quotes)
        write(fd, "\"", 1);
      write(fd, v_str, strlen(v_str));
      if (needs_quotes)
        write(fd, "\"", 1);

      // Use ARC release safely on the allocated C-string
      if (!is_str && v) {
        moksha_rt_release(v_str);
      }

      if (i < cols - 1)
        write(fd, ",", 1);
    }
    write(fd, "\n", 1);
  }
  close(fd);
}

MokshaAnyRet moksha_file_readCsv(char *path) {
  MokshaSlice *empty = (MokshaSlice *)moksha_rt_alloc(sizeof(MokshaSlice), 18);
  empty->data = NULL;
  empty->length = 0;

  if (!path) {
    return moksha_pack_any(empty, &vtable_array);
  }

  int fd = open(path, O_RDONLY | O_BINARY);
  if (fd < 0) {
    return moksha_pack_any(empty, &vtable_array);
  }

  off_t size = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  char *text = (char *)malloc(size + 1);
  int bytes_read = read(fd, text, size);
  close(fd);

  if (bytes_read < 0) {
    free(text);
    return moksha_pack_any(empty, &vtable_array);
  }
  text[bytes_read] = '\0';

  size_t cap = 16;
  size_t count = 0;
  MokshaAny *arr = (MokshaAny *)moksha_rt_alloc(cap * sizeof(MokshaAny), 18);

  char *p = text;
  char *headers[256];
  int cols = 0;

  while (*p == ' ' || *p == '\r' || *p == '\n')
    p++;

  // 1. Parse Headers
  while (*p && *p != '\n' && *p != '\r' && cols < 256) {
    char *start = p;
    if (*p == '"') {
      p++;
      start = p;
      while (*p && *p != '"')
        p++;
      int len = p - start;
      headers[cols] = make_mstring(start, len);
      if (*p == '"')
        p++;
    } else {
      while (*p && *p != ',' && *p != '\n' && *p != '\r')
        p++;
      int len = p - start;
      headers[cols] = make_mstring(start, len);
    }
    cols++;
    if (*p == ',')
      p++;
  }

  if (*p == '\r')
    p++;
  if (*p == '\n')
    p++;

  // 2. Parse Rows
  while (*p) {
    if (*p == '\n' || *p == '\r') {
      p++;
      continue;
    }

    void *map = moksha_rt_map_new();

    for (int i = 0; i < cols; i++) {
      if (!*p || *p == '\n' || *p == '\r')
        break;

      char *start = p;
      if (*p == '"') {
        p++;
        start = p;
        while (*p && *p != '"')
          p++;
        int len = p - start;

        MokshaAny *kp = moksha_box_string(headers[i]);
        MokshaAny *vp = parse_and_box_literal(start, len, true);
        moksha_rt_map_insert(map, kp, vp);

        if (*p == '"')
          p++;
      } else {
        while (*p && *p != ',' && *p != '\n' && *p != '\r')
          p++;
        int len = p - start;

        MokshaAny *kp = moksha_box_string(headers[i]);
        MokshaAny *vp = parse_and_box_literal(start, len, false);
        moksha_rt_map_insert(map, kp, vp);
      }

      if (*p == ',')
        p++;
    }

    if (count >= cap) {
      cap *= 2;
      MokshaAny *new_arr =
          (MokshaAny *)moksha_rt_alloc(cap * sizeof(MokshaAny), 18);
      memcpy(new_arr, arr, count * sizeof(MokshaAny));
      arr = new_arr;
    }

    arr[count].data = map;
    arr[count].vtable = &vtable_map;
    count++;

    while (*p && *p != '\n' && *p != '\r')
      p++;
    if (*p == '\r')
      p++;
    if (*p == '\n')
      p++;
  }

  free(text);

  // Single-box array return ABI
  MokshaSlice *slice = (MokshaSlice *)moksha_rt_alloc(sizeof(MokshaSlice), 18);
  slice->data = arr;
  slice->length = count;

  return moksha_pack_any(slice, &vtable_array);
}

// ============================================================================
// PDF endpoints
// ============================================================================

MokshaAnyRet moksha_file_createPdf(char *path) {
  MokshaAnyRet ret = moksha_file_open(path, 2 | 16 | 32);
  MokshaAny temp = {(void *)(uintptr_t)ret,
                    (const AnyVTable *)(uintptr_t)(ret >> 64)};
  int fd = unbox_fd(&temp);

  if (fd >= 0) {
    const char *magic = "%PDF-1.4\n%\xE2\xE3\xCF\xD3\n";
    write(fd, magic, strlen(magic));
  }
  return ret;
}

void moksha_file_writePdfText(MokshaAny *pdf_any, char *text) {
  moksha_file_writeLine(pdf_any, text);
}

void moksha_file_savePdf(MokshaAny *pdf_any) { moksha_file_close(pdf_any); }

MokshaAnyRet moksha_file_openPdf(char *path) {
  return moksha_file_open(path, 1);
}

char *moksha_file_extractText(MokshaAny *pdf_any) {
  MokshaAnyRet result_ret = moksha_file_read(pdf_any);
  char *raw_buffer = (char *)(uintptr_t)result_ret;

  if (!raw_buffer)
    return NULL;

  const char *magic = "%PDF-1.4\n%\xE2\xE3\xCF\xD3\n";
  size_t magic_len = strlen(magic);

  if (strncmp(raw_buffer, magic, magic_len) == 0) {
    char *text_start = raw_buffer + magic_len;
    size_t text_len = strlen(text_start);

    if (text_len > 0 && text_start[text_len - 1] == '\n') {
      text_len--;
    }

    char *extracted = (char *)moksha_rt_alloc(text_len + 1, 16);
    memcpy(extracted, text_start, text_len);
    extracted[text_len] = '\0';
    return extracted;
  }

  return raw_buffer;
}

// ============================================================================
// Directory Operations
// ============================================================================
bool moksha_file_createDir(char *path) {
  if (!path)
    return false;
#ifdef _WIN32
  return mkdir(path) == 0;
#else
  return mkdir(path, 0777) == 0;
#endif
}

bool moksha_file_isDir(char *path) {
  struct stat buffer;
  if (stat(path, &buffer) != 0)
    return false;
  return S_ISDIR(buffer.st_mode);
}

bool moksha_file_isFile(char *path) {
  struct stat buffer;
  if (stat(path, &buffer) != 0)
    return false;
  return S_ISREG(buffer.st_mode);
}

bool moksha_file_copy(char *src, char *dst) {
  if (!src || !dst)
    return false;
  int source = open(src, O_RDONLY);
  int target = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0666);

  if (source < 0 || target < 0) {
    if (source >= 0)
      close(source);
    if (target >= 0)
      close(target);
    return false;
  }

  char buf[4096];
  ssize_t n;
  while ((n = read(source, buf, sizeof(buf))) > 0) {
    write(target, buf, n);
  }

  close(source);
  close(target);
  return true;
}

bool moksha_file_move(char *src, char *dst) {
  if (!src || !dst)
    return false;
  return rename(src, dst) == 0;
}

bool moksha_file_remove(char *path) {
  if (!path)
    return false;
  return unlink(path) == 0;
}

bool moksha_file_removeDir(char *path) {
  if (!path)
    return false;
  return rmdir(path) == 0;
}

MokshaAnyRet moksha_file_listDir(char *path) {
  if (!path)
    return moksha_pack_any(NULL, NULL);

  DIR *dir = opendir(path);
  if (!dir) {
    MokshaSlice *empty = (MokshaSlice *)moksha_rt_alloc(sizeof(MokshaSlice), 2);
    empty->data = NULL;
    empty->length = 0;
    return moksha_pack_any(empty, &vtable_array);
  }

  size_t cap = 16;
  size_t len = 0;
  char **arr = (char **)moksha_rt_alloc(cap * sizeof(char *), 2);

  struct dirent *ent;
  while ((ent = readdir(dir)) != NULL) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
      continue;
    }
    if (len >= cap) {
      cap *= 2;
      char **new_arr = (char **)moksha_rt_alloc(cap * sizeof(char *), 2);
      memcpy(new_arr, arr, len * sizeof(char *));
      arr = new_arr;
    }
    size_t name_len = strlen(ent->d_name);
    arr[len++] = make_mstring(ent->d_name, name_len);
  }
  closedir(dir);

  MokshaSlice *slice = (MokshaSlice *)moksha_rt_alloc(sizeof(MokshaSlice), 2);
  slice->data = arr;
  slice->length = len;
  return moksha_pack_any(slice, &vtable_array);
}
