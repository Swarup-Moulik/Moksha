add_library(MokshaSupport
    diagnostics/Diagnostics.cpp
)

target_link_libraries(MokshaSupport
    PUBLIC
    mokshaSettings
    LLVMSupport
)
