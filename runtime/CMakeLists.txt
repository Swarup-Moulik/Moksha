cmake_minimum_required(VERSION 3.15)
project(MokshaRuntime C ASM) # Enable C and Assembly support

# ==============================================================================
# 1. Target Selection System
# Usage: cmake -DMOKSHA_TARGET=baremetal -DMOKSHA_ARCH=riscv32 -DMOKSHA_PLATFORM=qemu_virt ..
# ==============================================================================
set(MOKSHA_TARGET "host" CACHE STRING "Target platform (windows, linux, darwin, wasi, wasm_web, baremetal)")

# 2. Detect Host if no target specified
if(MOKSHA_TARGET STREQUAL "host")
  if(WIN32)
    set(MOKSHA_TARGET "windows")
  elseif(APPLE)
    set(MOKSHA_TARGET "darwin")
  else()
    set(MOKSHA_TARGET "linux")
  endif()
endif()

message(STATUS "Moksha Runtime Target: ${MOKSHA_TARGET}")

# ==============================================================================
# 3. Core (Platform-Independent) Sources
# ==============================================================================
set(CORE_SOURCES
    src/core/arc.c
    src/core/array.c
    src/core/string.c
    src/core/math.c
    src/core/panic.c
    src/core/map.c
    src/core/sync.c
    src/core/io.c
    src/core/file.c
    src/core/box.c
    src/core/exceptions.c
    src/core/channel.c
    src/memory/allocator.c
    src/async/task.c
    src/async/scheduler.c
    src/async/async_builtins.c
    src/debug/log.c
    src/debug/trace.c
)

# Only include the hosted runtime initialization for standard operating systems!
if(NOT MOKSHA_TARGET STREQUAL "baremetal")
  list(APPEND CORE_SOURCES src/core/runtime_init.c)
endif()

# ==============================================================================
# 4. Platform-Specific (ABI Implementation) Sources
# ==============================================================================
set(SYS_SOURCES "")

if(MOKSHA_TARGET STREQUAL "windows")
  list(APPEND SYS_SOURCES
        src/sys/windows/io.c
        src/sys/windows/alloc.c
        src/sys/windows/thread.c
        src/sys/windows/caps.c
        src/sys/windows/event_loop.c
        src/sys/windows/time.c
        src/sys/windows/process.c
    )
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  set(PLATFORM_LIBS kernel32)

elseif(MOKSHA_TARGET STREQUAL "linux")
  list(APPEND SYS_SOURCES
        src/sys/posix_common/io.c
        src/sys/posix_common/alloc.c
        src/sys/posix_common/thread.c
        src/sys/posix_common/time.c
        src/sys/linux/event_loop.c
        src/sys/linux/caps.c
        src/sys/posix_common/process.c
    )
  set(PLATFORM_LIBS pthread)

elseif(MOKSHA_TARGET STREQUAL "darwin")
  list(APPEND SYS_SOURCES
        src/sys/posix_common/io.c
        src/sys/posix_common/alloc.c
        src/sys/posix_common/thread.c
        src/sys/posix_common/time.c
        src/sys/posix_common/process.c
        src/sys/darwin/event_loop.c
        src/sys/darwin/caps.c
    )

elseif(MOKSHA_TARGET STREQUAL "wasi")
  list(APPEND SYS_SOURCES
        src/sys/wasi/io.c
        src/sys/wasi/alloc.c
        src/sys/wasi/event_loop.c
        src/sys/wasi/caps.c
        src/sys/posix_common/time.c
        src/sys/posix_common/process.c
    )

elseif(MOKSHA_TARGET STREQUAL "wasm_web")
  list(APPEND SYS_SOURCES
        src/sys/wasm_web/io.c
        src/sys/wasm_web/caps.c
        src/sys/wasm_web/event_loop.c
        src/sys/posix_common/time.c
        src/sys/posix_common/process.c
    )

elseif(MOKSHA_TARGET STREQUAL "baremetal")
  # Baremetal requires Architecture and Platform selection
  # Supported ARCH: x86_64, x86, aarch64, arm, riscv32, riscv64
  # Supported PLATFORM: qemu_virt, pc_bios_i686, pc_bios_x86_64, uefi, stm32
  set(MOKSHA_ARCH "x86_64" CACHE STRING "Baremetal Architecture")
  set(MOKSHA_PLATFORM "qemu_virt" CACHE STRING "Baremetal Platform/Board")

  message(STATUS "Baremetal Arch: ${MOKSHA_ARCH}")
  message(STATUS "Baremetal Platform: ${MOKSHA_PLATFORM}")

  # Core baremetal C sources (always required)
  list(APPEND SYS_SOURCES
            src/sys/baremetal/core/event_loop.c
            src/sys/baremetal/core/caps.c
      )

  if(MOKSHA_PLATFORM STREQUAL "uefi")
    # UEFI provides its own allocation and IO protocols
    list(APPEND SYS_SOURCES
              src/sys/baremetal/platform/uefi/uefi_alloc.c
              src/sys/baremetal/platform/uefi/uefi_io.c
              src/sys/baremetal/platform/uefi/uefi_main.c
          )
  else()
    # Standard BIOS/Hardware baremetal uses the default allocator and arch-specific IO
    list(APPEND SYS_SOURCES
              src/sys/baremetal/core/alloc.c
              src/sys/baremetal/arch/${MOKSHA_ARCH}/io.c
          )
  endif()

  # Flexible/Optional Architecture & Platform Files
  set(POTENTIAL_BAREMETAL_FILES
            src/sys/baremetal/arch/${MOKSHA_ARCH}/context.S
            src/sys/baremetal/platform/${MOKSHA_PLATFORM}/board.c
            src/sys/baremetal/platform/${MOKSHA_PLATFORM}/boot.S
            src/sys/baremetal/platform/${MOKSHA_PLATFORM}/multiboot.S
            src/sys/baremetal/platform/${MOKSHA_PLATFORM}/vga_text.c
            src/sys/baremetal/platform/${MOKSHA_PLATFORM}/idt.c
            src/sys/baremetal/platform/${MOKSHA_PLATFORM}/system.c
            src/sys/baremetal/platform/${MOKSHA_PLATFORM}/vectors.c
      )

  foreach(file ${POTENTIAL_BAREMETAL_FILES})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
      list(APPEND SYS_SOURCES ${file})
    endif()
  endforeach()

  # Core freestanding flags for bare-metal targets
  add_compile_options(-ffreestanding -nostdlib -fno-stack-protector -D__MOKSHA_BAREMETAL__)
endif()

# ==============================================================================
# 5. Create the Library
# ==============================================================================
add_library(moksha_rt STATIC ${CORE_SOURCES} ${SYS_SOURCES})

# ==============================================================================
# 6. Include Directories
# ==============================================================================
target_include_directories(moksha_rt PUBLIC
    include
    src       # For internal ABI headers
)

# ==============================================================================
# 7. Compiler-Specific Settings
# ==============================================================================
if(MSVC)
  # /EHa enables asynchronous/SEH exception unwinding in MSVC
  target_compile_options(moksha_rt PRIVATE /W4 /Z7 /EHa)
else()
  # -fexceptions forces GCC/Clang to emit unwind tables for C code
  target_compile_options(moksha_rt PRIVATE -Wall -Wextra -g -fexceptions)
endif()

# ==============================================================================
# 8. Output Naming (Produces libmoksha_rt_<target>.a)
# Matches what mokshac.cpp's resolveRuntimeLibrary expects!
# ==============================================================================
set_target_properties(moksha_rt PROPERTIES OUTPUT_NAME "moksha_rt_${MOKSHA_TARGET}")
