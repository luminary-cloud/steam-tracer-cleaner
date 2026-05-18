cmake_minimum_required(VERSION 3.20)

project(steam-tracer-cleaner
    VERSION 0.1.4
    DESCRIPTION "Cleans Steam account residue and installs autoexec / video configs"
    LANGUAGES C CXX
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed. Configure into a separate build/ directory.")
endif()

if(NOT WIN32)
    message(FATAL_ERROR "${PROJECT_NAME} targets Windows only.")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

if(MSVC)
    # CXX-only — otherwise /std:c++latest collides with /std:c11 on the vendored SQLite C amalgamation.
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/std:c++latest>)
endif()

# Static CRT so the binary runs without VC++ Redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(${CMAKE_CURRENT_LIST_DIR}/cmake/compile_options.cmake)

option(STC_BUILD_TESTS "Build unit tests" ON)

add_subdirectory(third_party)
add_subdirectory(platform)
add_subdirectory(core)
add_subdirectory(ui)
add_subdirectory(app)

if(STC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
