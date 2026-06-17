cmake_minimum_required(VERSION 3.15)
project(libbsc C CXX)
add_library(
    libbsc
    STATIC
    adler32.cpp
    bsc.cpp
    bsc_str_array.cpp
    bwt.cpp
    coder.cpp
    detectors.cpp
    libbsc.cpp
    libsais.c
    lzp.cpp
    platform.cpp
    preprocessing.cpp
    qlfc.cpp
    qlfc_model.cpp
    st.cpp
)
target_include_directories(
    libbsc
    PUBLIC . .. ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/common
)
set_target_properties(libbsc PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(TARGET OpenMP::OpenMP_CXX)
    target_link_libraries(libbsc PUBLIC OpenMP::OpenMP_CXX)
endif()

set(SPRING_LIBBSC_FLAG_TEXT "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS}")
foreach(
    flag_var
    CMAKE_C_FLAGS_DEBUG
    CMAKE_C_FLAGS_RELEASE
    CMAKE_C_FLAGS_RELWITHDEBINFO
    CMAKE_C_FLAGS_MINSIZEREL
    CMAKE_CXX_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_RELWITHDEBINFO
    CMAKE_CXX_FLAGS_MINSIZEREL
)
    if(DEFINED ${flag_var})
        string(APPEND SPRING_LIBBSC_FLAG_TEXT " ${${flag_var}}")
    endif()
endforeach()

if(SPRING_LIBBSC_FLAG_TEXT MATCHES "(^|[ 	])(-|/)fsanitize=")
    target_compile_definitions(libbsc PRIVATE LIBBSC_NO_UNALIGNED_ACCESS)
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
    target_compile_options(libbsc PRIVATE -Wno-unused-variable)
endif()

# Detect ClangCL (Clang with MSVC frontend, clang-cl.exe)
set(SPRING_LIBBSC_IS_CLANGCL FALSE)
if(WIN32 AND CMAKE_C_COMPILER_ID STREQUAL "Clang")
    get_filename_component(_libbsc_compiler_name "${CMAKE_C_COMPILER}" NAME_WE)
    if(_libbsc_compiler_name STREQUAL "clang-cl")
        set(SPRING_LIBBSC_IS_CLANGCL TRUE)
    endif()
endif()

# Workaround for MSVC ARM64 internal compiler error in libsais.c
# Use /Od to avoid compiler bug (OpenMP is disabled globally for MSVC ARM64)
if(MSVC AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|aarch64)$")
    foreach(
        flag_var
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS_MINSIZEREL
    )
        if(DEFINED ${flag_var})
            string(REPLACE "/O2" "" ${flag_var} "${${flag_var}}")
            string(REPLACE "/O1" "" ${flag_var} "${${flag_var}}")
            string(REPLACE "/Ox" "" ${flag_var} "${${flag_var}}")
            string(REPLACE "/Od" "" ${flag_var} "${${flag_var}}")
            string(REGEX REPLACE "[ 	]+" " " ${flag_var} "${${flag_var}}")
            string(STRIP "${${flag_var}}" ${flag_var})
        endif()
    endforeach()
    set_source_files_properties(libsais.c PROPERTIES COMPILE_OPTIONS "/Od")
endif()

# Workaround for ClangCL miscompilation of libbsc on Windows x64.
# ClangCL with /O2 miscompiles libsais.c (suffix array construction) for
# complex real-world inputs (varied sequences, complex read IDs), causing
# access violations (0xC0000005). Simple/uniform inputs happen to not
# trigger the affected code path, which is why integration tests with
# synthetic data pass. Override per-config flags in subdirectory scope.
if(
    SPRING_LIBBSC_IS_CLANGCL
    AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|aarch64)$"
)
    foreach(cfg RELEASE RELWITHDEBINFO MINSIZEREL)
        string(
            REGEX REPLACE "/O[0-9dtx]"
            "/Od"
            CMAKE_C_FLAGS_${cfg}
            "${CMAKE_C_FLAGS_${cfg}}"
        )
        string(
            REGEX REPLACE "/O[0-9dtx]"
            "/Od"
            CMAKE_CXX_FLAGS_${cfg}
            "${CMAKE_CXX_FLAGS_${cfg}}"
        )
    endforeach()
    message(
        NOTICE
        "Using /Od (no optimization) for libbsc on Windows ClangCL x64 to avoid access violations"
    )
endif()
