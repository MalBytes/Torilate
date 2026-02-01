cmake_minimum_required(VERSION 3.20)

project(Torilate
    VERSION 0.1.2
    LANGUAGES C
)

# ---- Language Standard ----
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ---- Executable ----
add_executable(torilate
    src/torilate.c
    src/cli/cli.c
    src/http/http.c
    src/util/util.c
    src/socks/socks4.c
    lib/argtable3/argtable3.c
)

# ---- Include paths ----
target_include_directories(torilate PRIVATE src)
target_include_directories(torilate PRIVATE lib)

# ---- Math library (libm) ----
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    target_link_libraries(torilate PUBLIC ${MATH_LIBRARY})
endif()

# ---- Platform-specific network package linking----
if (WIN32)
    target_link_libraries(torilate PRIVATE ws2_32)
    target_sources(torilate PRIVATE src/net/socket_win32.c)
else()
    target_sources(torilate PRIVATE src/net/socket_posix.c)
endif()

# ---- Output directory ----
set_target_properties(torilate PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

# ---- Warnings (compiler-aware) ----
if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
    CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(torilate PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# ---- Debug-friendly default ----
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()
