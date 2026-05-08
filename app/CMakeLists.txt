add_executable(steam-tracer-cleaner WIN32
    win_main.cpp
    app_state.cpp
    ${CMAKE_SOURCE_DIR}/app.rc
)

target_include_directories(steam-tracer-cleaner PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(steam-tracer-cleaner PRIVATE
    stc_ui
    stc_core
    stc_platform
    stc_imgui
    stc_spdlog
    d3d11
    dxgi
    dwmapi
    Comctl32
)

stc_set_compile_options(steam-tracer-cleaner)

set_target_properties(steam-tracer-cleaner PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_NAME "steam-tracer-cleaner"
)
