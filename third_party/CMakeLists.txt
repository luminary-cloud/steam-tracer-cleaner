# Vendored third-party libraries. No external package manager required.

# Dear ImGui (docking branch). Compiled as a static lib so warnings stay quiet.
add_library(stc_imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_win32.cpp
    imgui/backends/imgui_impl_dx11.cpp
)
target_include_directories(stc_imgui PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/imgui
    ${CMAKE_CURRENT_LIST_DIR}/imgui/backends
)
if(MSVC)
    target_compile_options(stc_imgui PRIVATE /W0)
endif()

# spdlog. Header-only, with std::format used so we don't drag fmt in.
add_library(stc_spdlog INTERFACE)
target_include_directories(stc_spdlog INTERFACE ${CMAKE_CURRENT_LIST_DIR}/spdlog/include)
target_compile_definitions(stc_spdlog INTERFACE SPDLOG_HEADER_ONLY SPDLOG_USE_STD_FORMAT)

# nlohmann::json - single-header
add_library(stc_json INTERFACE)
target_include_directories(stc_json INTERFACE ${CMAKE_CURRENT_LIST_DIR}/nlohmann_json/single_include)

# doctest - single-header testing framework
add_library(stc_doctest INTERFACE)
target_include_directories(stc_doctest INTERFACE ${CMAKE_CURRENT_LIST_DIR}/doctest)

# SQLite amalgamation. Compiled as C, warnings off, threadsafe enabled, no dl/load extension.
add_library(stc_sqlite STATIC sqlite/sqlite3.c)
target_include_directories(stc_sqlite PUBLIC ${CMAKE_CURRENT_LIST_DIR}/sqlite)
target_compile_definitions(stc_sqlite PRIVATE
    SQLITE_OMIT_LOAD_EXTENSION
    SQLITE_THREADSAFE=1
    SQLITE_DEFAULT_MEMSTATUS=0
)
if(MSVC)
    target_compile_options(stc_sqlite PRIVATE /W0)
endif()
