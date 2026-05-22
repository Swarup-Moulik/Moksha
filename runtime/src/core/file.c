#include "../../include/moksha_rt.h"
#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
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
extern void moksha_mem_free(void *ptr);

// External map & string bindings needed for structured data
extern int32_t moksha_rt_map_len(void *map);
extern MokshaAny *moksha_rt_map_get_key_at(void *map, int32_t index);
extern MokshaAny *moksha_rt_map_get_val_at(void *map, int32_t index);
extern char *__moksha_any_to_string(MokshaAny *any_val);
extern void *moksha_rt_map_new();
extern void moksha_rt_map_insert(void *map, MokshaAny *key, MokshaAny *val);
extern const AnyVTable vtable_map;

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
  return (int)*(intptr_t *)(file_any->data);
}

int32_t length(void *arr_ptr) {
  if (!arr_ptr)
    return 0;

  // 1. The compiler passes 'any' objects by pointer
  MokshaAny *any_val = (MokshaAny *)arr_ptr;
  if (!any_val->data)
    return 0;

  // 2. Unbox the actual slice data from the any container
  MokshaSlice *slice = (MokshaSlice *)any_val->data;
  return (int32_t)slice->length;
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

  // Trim trailing whitespace (common in naive YAML loops)
  while (val_len > 0 &&
         (val_start[val_len - 1] == ' ' || val_start[val_len - 1] == '\r')) {
    val_len--;
  }

  // Create a null-terminated stack buffer for safe C standard library parsing
  char tmp[128];
  int copy_len = val_len < 127 ? val_len : 127;
  memcpy(tmp, val_start, copy_len);
  tmp[copy_len] = '\0';

  // Parse Booleans
  if (strncmp(tmp, "true", 4) == 0)
    return moksha_box_bool(true);
  if (strncmp(tmp, "false", 5) == 0)
    return moksha_box_bool(false);
  if (strncmp(tmp, "null", 4) == 0)
    return moksha_box_string(""); // Naive null fallback

  // Parse Numbers (Detect Floats vs Ints)
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
    // If strtod consumed characters, it's a valid float
    if (endptr != tmp) {
      return moksha_box_f64(val);
    }
  } else {
    char *endptr;
    long val = strtol(tmp, &endptr, 10);
    // If strtol consumed characters, it's a valid integer
    if (endptr != tmp) {
      return moksha_box_i32((int32_t)val);
    }
  }

  // Fallback: If it's an unquoted string in YAML that isn't a number
  // Recursively treat it as a string
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

  char *str = (char *)data_any->data;
  write(fd, str, strlen(str));
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

  return moksha_pack_any(buf, NULL);
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

  // FIX: Track if any characters were actually read to avoid stack garbage bugs
  // at EOF
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

MokshaAnyRet moksha_file_readLines(MokshaAny *file_any) {
  if (!file_any)
    return moksha_pack_any(NULL, NULL);

  MokshaAny target_file = *file_any;
  bool should_close = false;

  // FIX: Polymorphic detection. If type_id == 16 (MOKSHA_TYPE_STRING),
  // treat the input as a high-level string path and manage the file handle
  // internally.
  if (file_any->vtable && file_any->vtable->type_id == 16) {
    char *path = (char *)file_any->data;
    MokshaAnyRet open_ret =
        moksha_file_open(path, 0); // Open in READ mode (O_RDONLY)

    target_file.data = (void *)(uintptr_t)open_ret;
    target_file.vtable = (void *)(uintptr_t)(open_ret >> 64);

    if (!target_file.data)
      return moksha_pack_any(NULL, NULL);

    should_close = true;
  }

  size_t cap = 16;
  size_t count = 0;
  char **arr = (char **)moksha_rt_alloc(cap * sizeof(char *), 2);

  while (true) {
    char *line = moksha_file_readLine(&target_file);
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

  // Clean up handle if opened dynamically
  if (should_close) {
    moksha_file_close(&target_file);
  }

  MokshaSlice *slice = (MokshaSlice *)moksha_rt_alloc(sizeof(MokshaSlice), 2);
  slice->data = arr;
  slice->length = count;
  return moksha_pack_any(slice, NULL);
}

// ============================================================================
// High-Level File Path Methods
// ============================================================================

bool moksha_file_exists(char *path) {
  if (!path)
    return false;
  return access(path, F_OK) == 0;
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

char *moksha_file_readText(char *path) {
  if (!path)
    return NULL;

  MokshaAnyRet ret = moksha_file_open(path, 0);
  MokshaAny file_any = {(void *)(uintptr_t)ret, (void *)(uintptr_t)(ret >> 64)};
  if (!file_any.data)
    return NULL;

  MokshaAnyRet result_ret = moksha_file_read(&file_any);
  moksha_file_close(&file_any);

  return (char *)(uintptr_t)result_ret; // Low 64 bits = data
}

void moksha_file_writeBytes(char *path, void *bytes_any_ptr) {
  if (!path || !bytes_any_ptr)
    return;

  MokshaAny *any_val = (MokshaAny *)bytes_any_ptr;
  if (!any_val->data)
    return;

  MokshaSlice *slice = (MokshaSlice *)any_val->data;

  // Use O_BINARY to prevent Windows from corrupting CRLF bytes
  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
  if (fd >= 0) {
    write(fd, slice->data, slice->length);
    close(fd);
  }
}

void moksha_file_appendBytes(char *path, void *bytes_any_ptr) {
  if (!path || !bytes_any_ptr)
    return;

  MokshaAny *any_val = (MokshaAny *)bytes_any_ptr;
  if (!any_val->data)
    return;

  MokshaSlice *slice = (MokshaSlice *)any_val->data;

  int fd = open(path, O_WRONLY | O_CREAT | O_APPEND | O_BINARY, 0666);
  if (fd >= 0) {
    write(fd, slice->data, slice->length);
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

  return moksha_pack_any(slice, NULL);
}

// ============================================================================
// Structured Data (JSON / YAML / PDF File Streamers)
// ============================================================================

void moksha_file_writeJson(char *path, void *data_any_ptr) {
  MokshaAny *any_val = (MokshaAny *)data_any_ptr;
  if (!any_val || !any_val->data)
    return;

  void *map_ptr = any_val->data;
  int len = moksha_rt_map_len(map_ptr);

  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
  if (fd < 0)
    return;

  write(fd, "{\n", 2);
  for (int i = 0; i < len; i++) {
    MokshaAny *k = moksha_rt_map_get_key_at(map_ptr, i);
    MokshaAny *v = moksha_rt_map_get_val_at(map_ptr, i);

    char *k_str = (k && k->data) ? (char *)k->data : "";

    // FIX: Bypass __moksha_any_to_string for strings to prevent memory address
    // leaks
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

    // Read Key
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

    // Read Value
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

    // 1. Box Key
    char *key_str = moksha_rt_alloc(key_len + 1, MOKSHA_TYPE_STRING);
    memcpy(key_str, key_start, key_len);
    key_str[key_len] = '\0';
    MokshaAny *kp = moksha_box_string(key_str);

    // 2. Box Value using universal literal parser
    MokshaAny *vp = parse_and_box_literal(val_start, val_len, is_string);

    moksha_rt_map_insert(map, kp, vp);
  }
  return moksha_pack_any(map, &vtable_map);
}

void moksha_file_writeYaml(char *path, void *data_any_ptr) {
  MokshaAny *any_val = (MokshaAny *)data_any_ptr;
  if (!any_val || !any_val->data)
    return;

  void *map_ptr = any_val->data;
  int len = moksha_rt_map_len(map_ptr);

  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
  if (fd < 0)
    return;

  for (int i = 0; i < len; i++) {
    MokshaAny *k = moksha_rt_map_get_key_at(map_ptr, i);
    MokshaAny *v = moksha_rt_map_get_val_at(map_ptr, i);

    char *k_str = (k && k->data) ? (char *)k->data : "";

    // FIX: Bypass __moksha_any_to_string for strings to prevent memory address
    // leaks
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

    // Detect if YAML value is explicitly quoted (otherwise parsed dynamically)
    bool is_string = false;
    if (val_len >= 2 && val_start[0] == '"' && val_start[val_len - 1] == '"') {
      is_string = true;
      val_start++;  // Skip leading quote
      val_len -= 2; // Remove trailing quote from length
    }

    // 1. Box Key
    char *key_str = moksha_rt_alloc(key_len + 1, MOKSHA_TYPE_STRING);
    memcpy(key_str, key_start, key_len);
    key_str[key_len] = '\0';
    MokshaAny *kp = moksha_box_string(key_str);

    // 2. Box Value using universal literal parser
    MokshaAny *vp = parse_and_box_literal(val_start, val_len, is_string);

    moksha_rt_map_insert(map, kp, vp);
  }
  return moksha_pack_any(map, &vtable_map);
}

// PDF endpoints (Leaving identical structure, passing pointers)
MokshaAnyRet moksha_file_createPdf(char *path) {
  MokshaAnyRet ret = moksha_file_open(path, 2 | 16 | 32);
  MokshaAny file_any = {(void *)(uintptr_t)ret, (void *)(uintptr_t)(ret >> 64)};
  int fd = unbox_fd(&file_any);

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

  // Find where the actual text begins by skipping the PDF magic header.
  // The header we write is exactly 16 bytes: "%PDF-1.4\n%\xE2\xE3\xCF\xD3\n"
  const char *magic = "%PDF-1.4\n%\xE2\xE3\xCF\xD3\n";
  size_t magic_len = strlen(magic);

  if (strncmp(raw_buffer, magic, magic_len) == 0) {
    char *text_start = raw_buffer + magic_len;
    size_t text_len = strlen(text_start);

    // moksha_file_writeLine adds a trailing newline, trim it for exact matches
    if (text_len > 0 && text_start[text_len - 1] == '\n') {
      text_len--;
    }

    char *extracted =
        (char *)moksha_rt_alloc(text_len + 1, 16); // 16 = String type
    memcpy(extracted, text_start, text_len);
    extracted[text_len] = '\0';
    return extracted;
  }

  // Fallback: If it's not our mocked PDF, just return the raw buffer
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
    return moksha_pack_any(empty, NULL);
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
  return moksha_pack_any(slice, NULL);
}
