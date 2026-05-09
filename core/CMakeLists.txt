add_library(stc_core STATIC
    log.cpp
    vdf.cpp
    steam_paths.cpp
    appid_map.cpp
    targets.cpp
    ignore_list.cpp
    profile.cpp
    dry_run.cpp
    backup.cpp
    cleaner.cpp
    autoexec.cpp
    config_library.cpp
    steam_lifecycle.cpp
    audit.cpp
    browser_cookies.cpp
    crash_dumps.cpp
    scheduled_clean.cpp
)

target_include_directories(stc_core PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(stc_core PUBLIC
    stc_platform
    stc_spdlog
    stc_json
    stc_sqlite
)

stc_set_compile_options(stc_core)
