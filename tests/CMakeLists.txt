add_executable(stc_tests
    test_vdf.cpp
    test_ignore_list.cpp
    test_appid_map.cpp
    test_steam_paths.cpp
    test_dry_run.cpp
)

target_link_libraries(stc_tests PRIVATE
    stc_core
    stc_platform
    stc_doctest
)

stc_set_compile_options(stc_tests)

add_test(NAME stc_tests COMMAND stc_tests)
