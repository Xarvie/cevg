# ===================================================================
# Cevg VG — Skia Graphite / Vulkan (optional GPU backend)
# ===================================================================
# A clean, single-backend rewrite. Vulkan GPU backend is optional:
# when CEVG_VULKAN=ON (default) the library uses Skia Graphite on
# Vulkan; when CEVG_VULKAN=OFF it falls back to CPU-only rendering
# and does not require Vulkan headers or libraries. The GUI layer is
# not part of this project.
#
# Layout:
#   include/cevg/cevg.h           public C ABI
#   common/cevg_internal.h        shared Path/Paint struct bodies
#   common/cevg_common.c             backend-agnostic Path/Paint impl
#   src/cevg_skia_graphite.cpp       the Graphite/Vulkan backend
#   cevg.def                      MSVC export list
#
# Skia is compiled from source via add_subdirectory(third/skia).
# ===================================================================
cmake_minimum_required(VERSION 3.20)
project(cevg VERSION 0.2.0 LANGUAGES C CXX)

# Match Skia's prebuilt static CRT on MSVC.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(CEVG_SHARED "Build cevg as a shared library (DLL)" OFF)
option(CEVG_VULKAN "Enable Vulkan GPU backend (loaded at runtime via glad2)" ON)

# ---- Position-Independent Code (required when linking static libs into .so) ----
if(CEVG_SHARED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# ---- glad2: runtime Vulkan loader (no Vulkan SDK needed) ----
if(CEVG_VULKAN)
    add_library(cevg_glad STATIC "${CMAKE_CURRENT_SOURCE_DIR}/third/glad/src/vulkan.c")
    target_include_directories(cevg_glad PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/third/glad/include"   # <glad/vulkan.h> + <vulkan/vulkan.h> hijack
    )
    if(UNIX)
        target_link_libraries(cevg_glad PUBLIC ${CMAKE_DL_LIBS})
    endif()
endif()

# ---- Build Skia from source ----
set(SKIA_USE_VULKAN ${CEVG_VULKAN} CACHE BOOL "Build Skia with Vulkan support" FORCE)
add_subdirectory(third/skia)

# ===================================================================
# Library target
# ===================================================================
if(CEVG_SHARED)
    add_library(cevg SHARED)
    target_compile_definitions(cevg PRIVATE CEVG_EXPORTS)
else()
    add_library(cevg STATIC)
    target_compile_definitions(cevg PUBLIC CEVG_STATIC)
endif()

set_target_properties(cevg PROPERTIES PREFIX "")

target_include_directories(cevg PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"      # <cevg/cevg.h>
)
target_include_directories(cevg PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/common"       # "cevg_internal.h"
)

# Conditionally enable Vulkan backend.
if(CEVG_VULKAN)
    target_compile_definitions(cevg PRIVATE CEVG_VULKAN)
    target_link_libraries(cevg PRIVATE cevg_glad)
endif()

# ---- Compile cevg_common.c separately (plain C, no Skia includes) ----
add_library(cevg_common OBJECT "${CMAKE_CURRENT_SOURCE_DIR}/common/cevg_common.c")
target_include_directories(cevg_common PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
# cevg_common.c includes cevg.h which uses CEVG_API; when building the
# library (shared or static) the definitions must match the main target.
if(CEVG_SHARED)
    target_compile_definitions(cevg_common PRIVATE CEVG_EXPORTS)
else()
    target_compile_definitions(cevg_common PUBLIC CEVG_STATIC)
endif()
set_target_properties(cevg_common PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)
if(MSVC)
    target_compile_options(cevg_common PRIVATE /experimental:c11atomics)
endif()

target_sources(cevg PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cevg_skia_graphite.cpp"
    $<TARGET_OBJECTS:cevg_common>
)

# ---- Link Skia (built from source) ----
# Use PRIVATE so Skia's PUBLIC includes (ICU C++ headers etc.) don't
# leak into cevg_common.c which is plain C.  The C++ source file
# cevg_skia_graphite.cpp gets Skia includes via target_sources ordering.
target_link_libraries(cevg PRIVATE skia)

# Vulkan symbols are loaded at runtime by glad2 — no vulkan-1.lib needed.

# ---- Symbol visibility ----
# Hide all symbols by default; only CEVG_API-marked symbols are exported.
# Windows: __declspec(dllexport) auto-handles PE export, no extra steps.
# Linux/Android: -fvisibility=hidden + --version-script
# macOS/iOS: -fvisibility=hidden + -exported_symbols_list
set_target_properties(cevg PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
if(NOT WIN32)
    target_compile_options(cevg PRIVATE -fvisibility=hidden)
    if(CEVG_SHARED)
        if(APPLE)
            # macOS/iOS: export symbol list
            set(CEVG_EXPORTED_SYMBOLS "${CMAKE_CURRENT_SOURCE_DIR}/cevg.exported_symbols")
            target_link_options(cevg PRIVATE -exported_symbols_list "${CEVG_EXPORTED_SYMBOLS}")
        else()
            # Linux/Android: GNU ld version script
            target_link_options(cevg PRIVATE "SHELL:-Wl,--version-script ${CMAKE_CURRENT_SOURCE_DIR}/cevg.version")
        endif()
    endif()
endif()

# ---- Unified output directories ----
# All binaries (DLL, EXE) go to the same directory so DLLs are always
# found next to their consumers without PATH tricks or post-build copies.
set(CEVG_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")
set_target_properties(cevg PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         "${CEVG_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CEVG_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CEVG_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY         "${CEVG_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CEVG_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG   "${CEVG_OUTPUT_DIR}"
)

set_target_properties(cevg PROPERTIES
    PREFIX ""
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# MSVC needs /experimental:c11atomics for C11 <stdatomic.h> support.
if(MSVC)
    target_compile_options(cevg PRIVATE /experimental:c11atomics)
endif()

# ---- Static C++ runtime (ABI isolation: no libstdc++/libgcc dependency) ----
if(CEVG_SHARED AND NOT WIN32 AND NOT APPLE)
    target_link_options(cevg PRIVATE -static-libstdc++ -static-libgcc)
endif()

# ---- Warnings (browser-grade hygiene) ----
# On by default. -Werror is opt-in (CI) so a stray third-party-header warning
# never blocks a local build. Disable entirely with -DCEVG_STRICT_WARNINGS=OFF.
option(CEVG_STRICT_WARNINGS "Enable -Wall -Wextra (/W4 on MSVC)" ON)
option(CEVG_WARNINGS_AS_ERRORS "Treat warnings as errors (CI)" OFF)
if(CEVG_STRICT_WARNINGS)
    if(MSVC)
        target_compile_options(cevg PRIVATE /W4 $<$<BOOL:${CEVG_WARNINGS_AS_ERRORS}>:/WX>)
    else()
        target_compile_options(cevg PRIVATE -Wall -Wextra
            $<$<BOOL:${CEVG_WARNINGS_AS_ERRORS}>:-Werror>)
    endif()
endif()

# ---- ThreadSanitizer ----
# -DCEVG_SANITIZE_THREAD=ON instruments the library AND everything that links it
# (PUBLIC), so the multi-recorder / shared-paint / shared-image concurrency
# paths can be stress-tested under TSan (see tests/cevg_stress_threads.c). For a
# fully clean report, build Skia with TSan too; otherwise expect third-party
# noise that can be suppressed. GCC/Clang only.
option(CEVG_SANITIZE_THREAD "Build with ThreadSanitizer" OFF)
if(CEVG_SANITIZE_THREAD AND NOT MSVC)
    target_compile_options(cevg PUBLIC -fsanitize=thread -g -O1 -fno-omit-frame-pointer)
    target_link_options(cevg PUBLIC -fsanitize=thread)
endif()

# ---- Copy resources/ to output directory ----
add_custom_command(TARGET cevg POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        "${CEVG_OUTPUT_DIR}/resources"
    COMMENT "Copying resources/ to ${CEVG_OUTPUT_DIR}/resources"
)

# ===================================================================
# Optional: tests. Enable with -DCEVG_BUILD_TESTS=ON. The test
# harness must create the Vulkan device AND a VkSurfaceKHR for the
# window (the new present path needs a real surface). See tests/.
# ===================================================================
option(CEVG_BUILD_TESTS "Build Cevg VG tests" OFF)
if(CEVG_BUILD_TESTS)
    add_subdirectory(tests)
endif()
