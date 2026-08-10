#include "abi/sys_io.h"
#include <stdint.h>

#define COM1_PORT 0x3F8

static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

extern void vga_init(void);
extern void vga_write(const char *data, size_t size);

static int serial_initialized = 0;

static void init_serial(void) {
  outb(COM1_PORT + 1, 0x00);
  outb(COM1_PORT + 3, 0x80);
  outb(COM1_PORT + 0, 0x03);
  outb(COM1_PORT + 1, 0x00);
  outb(COM1_PORT + 3, 0x03);
  outb(COM1_PORT + 2, 0xC7);
  outb(COM1_PORT + 4, 0x0B);
  serial_initialized = 1;
}

static int is_transmit_empty(void) { return inb(COM1_PORT + 5) & 0x20; }

static void write_serial(char a) {
  while (is_transmit_empty() == 0)
    ;
  outb(COM1_PORT, a);
}

sys_err_t sys_io_write(int32_t fd, const void *buffer, size_t count,
                       size_t *out_bytes_written) {
  (void)fd; // We just write to Serial/VGA

  if (!serial_initialized) {
    init_serial();
    vga_init();
  }

  const char *buf = (const char *)buffer;
  for (size_t i = 0; i < count; i++) {
    write_serial(buf[i]);
  }

  vga_write(buf, count);

  if (out_bytes_written) {
    *out_bytes_written = count;
  }
  return SYS_OK;
}

sys_err_t sys_io_open(const char *path, int mode, int32_t *out_fd) {
  (void)path;
  (void)mode;
  (void)out_fd;
  return SYS_ERR_NOTSUP;
}
sys_err_t sys_io_read(int32_t fd, void *buffer, size_t count,
                      size_t *out_bytes_read) {
  (void)fd;
  (void)buffer;
  (void)count;
  (void)out_bytes_read;
  return SYS_ERR_NOTSUP;
}
sys_err_t sys_io_close(int32_t fd) {
  (void)fd;
  return SYS_ERR_NOTSUP;
}
