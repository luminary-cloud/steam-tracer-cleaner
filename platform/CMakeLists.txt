add_library(stc_platform STATIC
    paths.cpp
    registry.cpp
    process.cpp
    fs.cpp
)

target_include_directories(stc_platform PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(stc_platform PUBLIC
    stc_spdlog
    Shell32
    Advapi32
    Ole32
)

stc_set_compile_options(stc_platform)
