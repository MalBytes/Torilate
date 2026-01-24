cmake_minimum_required(VERSION 3.20)

project(Torilate
    VERSION 0.1.0
    LANGUAGES C
)

# ---- Language Standard ----
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# -------- Executables --------
add_executable(torilate torilate.c)

# -------- Linker Settings --------
if (WIN32)
    target_link_libraries(torilate PRIVATE ws2_32)
endif()

# -------- Output Directory --------
set_target_properties(torilate PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

# -------- warnings (compiler-aware) --------
if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(torilate PRIVATE -Wall -Wextra -Wpedantic)
endif()

# -------- debug-friendly defaults --------
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()