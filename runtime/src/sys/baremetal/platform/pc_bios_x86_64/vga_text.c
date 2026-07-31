#include <stddef.h>
#include <stdint.h>

// Standard VGA text mode constants
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t *const VGA_BUFFER = (uint16_t *)0xB8000;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;

// Colors: 7 = Light Grey, 0 = Black (Light Grey on Black)
#define VGA_COLOR_DEFAULT 0x07

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
  return (uint16_t)uc | (uint16_t)color << 8;
}

void vga_init(void) {
  terminal_row = 0;
  terminal_column = 0;
  terminal_color = VGA_COLOR_DEFAULT;
  for (size_t y = 0; y < VGA_HEIGHT; y++) {
    for (size_t x = 0; x < VGA_WIDTH; x++) {
      const size_t index = y * VGA_WIDTH + x;
      VGA_BUFFER[index] = vga_entry(' ', terminal_color);
    }
  }
}

static void vga_scroll(void) {
  // Move all rows up by one
  for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
    for (size_t x = 0; x < VGA_WIDTH; x++) {
      VGA_BUFFER[y * VGA_WIDTH + x] = VGA_BUFFER[(y + 1) * VGA_WIDTH + x];
    }
  }
  // Clear the last row
  for (size_t x = 0; x < VGA_WIDTH; x++) {
    VGA_BUFFER[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
        vga_entry(' ', terminal_color);
  }
  terminal_row = VGA_HEIGHT - 1;
}

void vga_putchar(char c) {
  if (c == '\n') {
    terminal_column = 0;
    if (++terminal_row == VGA_HEIGHT) {
      vga_scroll();
    }
    return;
  }

  const size_t index = terminal_row * VGA_WIDTH + terminal_column;
  VGA_BUFFER[index] = vga_entry((unsigned char)c, terminal_color);

  if (++terminal_column == VGA_WIDTH) {
    terminal_column = 0;
    if (++terminal_row == VGA_HEIGHT) {
      vga_scroll();
    }
  }
}

void vga_write(const char *data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    vga_putchar(data[i]);
  }
}
