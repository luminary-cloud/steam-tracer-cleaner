# Shared compile options. Applied via stc_set_compile_options(<target>).

function(stc_set_compile_options target)
    target_compile_definitions(${target} PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
        _CRT_SECURE_NO_WARNINGS
        _WIN32_WINNT=0x0A00      # Windows 10 minimum
    )

    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /Zc:__cplusplus
            /Zc:preprocessor
            /Zc:inline
            /utf-8
            /MP
            /EHsc
            /external:anglebrackets /external:W0
            $<$<CONFIG:Release>:/GL /Gw>
        )
        target_link_options(${target} PRIVATE
            $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
        )
        # Enable address sanitizer in Debug for the dev loop. Comment out if vcpkg deps don't ship asan.
        # target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:/fsanitize=address>)
    endif()
endfunction()
