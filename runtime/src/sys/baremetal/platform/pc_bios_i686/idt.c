#include <stdint.h>

// Forward declare the runtime panic function so we can crash gracefully
extern void moksha_rt_bug(const char *message);

// 64-bit IDT Entry
struct idt_entry {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t ist;
  uint8_t type_attr;
  uint16_t offset_mid;
  uint32_t offset_high;
  uint32_t zero;
} __attribute__((packed));

// IDT Pointer for the 'lidt' instruction
struct idt_ptr {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr idtp;

// Set an entry in the IDT
static void idt_set_gate(int num, uint64_t base, uint16_t sel, uint8_t flags) {
  idt[num].offset_low = (base & 0xFFFF);
  idt[num].offset_mid = (base >> 16) & 0xFFFF;
  idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
  idt[num].selector = sel;
  idt[num].ist = 0;
  idt[num].type_attr = flags;
  idt[num].zero = 0;
}

// Generic fault handler (No fancy registers dumped, just halts)
__attribute__((interrupt)) static void generic_isr_handler(void *frame) {
  (void)frame;
  moksha_rt_bug(
      "CPU EXCEPTION: Hardware Fault Detected (E.g. Null Deref, Div-by-Zero)");
}

void idt_init(void) {
  idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
  idtp.base = (uint64_t)&idt;

  // Clear the IDT
  for (int i = 0; i < 256; i++) {
    // 0x08 = Kernel Code Segment selector
    // 0x8E = Interrupt Gate, Present, DPL 0
    idt_set_gate(i, (uint64_t)generic_isr_handler, 0x08, 0x8E);
  }

  // Load the IDT into the CPU
  __asm__ volatile("lidt %0" : : "m"(idtp));
}
