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
# Apply conservative optimizations to avoid miscompilation in inflate/deflate
if(WIN32)
    set(needs_zlib_workaround FALSE)

    # MSVC on ARM64 needs the workaround
    if(
        CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|aarch64)$"
        AND CMAKE_C_COMPILER_ID STREQUAL "MSVC"
    )
        set(needs_zlib_workaround TRUE)
        set(workaround_reason "MSVC ARM64")
    endif()

    # ClangCL on any architecture needs the workaround
    if(
        CMAKE_C_COMPILER_ID STREQUAL "Clang"
        AND (
            MSVC
            OR CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"
            OR CMAKE_C_SIMULATE_ID STREQUAL "MSVC"
        )
    )
        set(needs_zlib_workaround TRUE)
        set(workaround_reason "ClangCL")
    endif()

    if(needs_zlib_workaround)
        # Use /Od (no optimization) to avoid access violations
        target_compile_options(spring_cloudflare_zlib PRIVATE /Od /Zi)
        message(
            NOTICE
            "Using /Od for zlib on Windows (${workaround_reason}) to avoid access violations"
        )
    endif()
endif()

set_target_properties(
    spring_cloudflare_zlib
    PROPERTIES POSITION_INDEPENDENT_CODE ON
)
add_library(ZLIB::ZLIB ALIAS spring_cloudflare_zlib)
