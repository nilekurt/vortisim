include(ClangTidy)

function(configure_tidy TARGET_NAME)
    if (VORTISIM_CLANG_TIDY)
        clang_tidy_check(${TARGET_NAME})
    endif()
endfunction()