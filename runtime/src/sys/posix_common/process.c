#include "../../abi/sys_process.h"
#include <stdlib.h>

void sys_process_exit(int32_t exit_code) {
  exit(exit_code); // Clean exit, no SIGILL
}

void sys_process_abort(void) {
  abort(); // Sends SIGABRT, dumping core for GDB
}
