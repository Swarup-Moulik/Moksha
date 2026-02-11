# Gather all runtime sources
file(GLOB RUNTIME_SOURCES "src/*.c")

# Create static library
add_library(mokshaRuntime STATIC ${RUNTIME_SOURCES})

# Include directories
target_include_directories(mokshaRuntime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Compiler options
if(MSVC)
    target_compile_options(mokshaRuntime PRIVATE /W4 /WX)
else()
    target_compile_options(mokshaRuntime PRIVATE -Wall -Wextra -Werror -pedantic -Wno-unused-parameter)
endif()
