#include "../../abi/sys_process.h"
#include <stdlib.h>
#include <windows.h>

void sys_process_exit(int32_t exit_code) { ExitProcess((UINT)exit_code); }

void sys_process_abort(void) {
  abort(); // Triggers the Windows error reporting / JIT debugger trap
}
