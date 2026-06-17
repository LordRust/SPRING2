cmake_minimum_required(VERSION 3.15)
project(spring_cloudflare_zlib C)
add_library(
    spring_cloudflare_zlib
    STATIC
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    inffast.c
    inflate.c
    inftrees.c
    trees.c
    uncompr.c
    zutil.c
)
target_include_directories(spring_cloudflare_zlib PUBLIC .)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
        spring_cloudflare_zlib
        PRIVATE -fno-lto -fno-strict-aliasing
    )
endif()
if(
    WIN32
    AND CMAKE_C_COMPILER_ID STREQUAL "Clang"
    AND (
        MSVC
        OR CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"
        OR CMAKE_C_SIMULATE_ID STREQUAL "MSVC"
    )
)
    target_compile_options(spring_cloudflare_zlib PRIVATE -Wno-unused-variable)
endif()

# Workaround for Windows zlib crashes with gzipped input on MSVC-like compilers
# The issue appears to be aggressive optimizations causing miscompilation in
# inflate/gzread code paths. Use conservative O1 instead of aggressive O2.
if(WIN32)
    set(needs_zlib_workaround FALSE)
    set(use_msvc_flag_syntax FALSE)

    # Determine if we should use MSVC flag syntax (/O1) or GCC syntax (-O1)
    # Only use MSVC syntax for actual MSVC or clang-cl.exe
    if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        set(use_msvc_flag_syntax TRUE)
    elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
        # Check if the compiler executable is clang-cl (not just any Clang)
        get_filename_component(compiler_name "${CMAKE_C_COMPILER}" NAME_WE)
        if(compiler_name STREQUAL "clang-cl")
            set(use_msvc_flag_syntax TRUE)
        endif()
    endif()

    # MSVC on ARM64 needs the workaround
    if(
        CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|aarch64)$"
        AND CMAKE_C_COMPILER_ID STREQUAL "MSVC"
    )
        set(needs_zlib_workaround TRUE)
        set(workaround_reason "MSVC ARM64")
    endif()

    # ClangCL on any architecture needs the workaround
    if(CMAKE_C_COMPILER_ID STREQUAL "Clang" AND use_msvc_flag_syntax)
        set(needs_zlib_workaround TRUE)
        set(workaround_reason "ClangCL")
    endif()

    # Regular Clang on Windows (not ClangCL) also needs the workaround
    if(CMAKE_C_COMPILER_ID STREQUAL "Clang" AND NOT use_msvc_flag_syntax)
        set(needs_zlib_workaround TRUE)
        set(workaround_reason "Windows Clang")
    endif()

    if(needs_zlib_workaround)
        if(use_msvc_flag_syntax)
            # Use /O1 (optimize for size) for MSVC and ClangCL
            target_compile_options(spring_cloudflare_zlib PRIVATE /O1)
            message(
                NOTICE
                "Using /O1 for zlib on Windows (${workaround_reason}) to avoid access violations"
            )
        else()
            # Use -O1 for regular Clang on Windows
            target_compile_options(spring_cloudflare_zlib PRIVATE -O1)
            message(
                NOTICE
                "Using -O1 for zlib on Windows (${workaround_reason}) to avoid access violations"
            )
        endif()
    endif()
endif()

set_target_properties(
    spring_cloudflare_zlib
    PROPERTIES POSITION_INDEPENDENT_CODE ON
)
add_library(ZLIB::ZLIB ALIAS spring_cloudflare_zlib)
