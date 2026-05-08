add_library(stc_ui STATIC
    theme.cpp
    fonts.cpp
    main_window.cpp
    screens/cleaner_screen.cpp
    screens/configs_screen.cpp
    screens/audit_screen.cpp
    screens/backups_screen.cpp
    screens/settings_screen.cpp
    widgets/account_picker.cpp
    widgets/target_tree.cpp
)

target_include_directories(stc_ui PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(stc_ui PUBLIC
    stc_core
    stc_platform
    stc_imgui
    stc_spdlog
    Comdlg32
)

stc_set_compile_options(stc_ui)
