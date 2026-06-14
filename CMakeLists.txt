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
    target_compile_definitions(cevg PRIVATE CEVG_EXPORTS PUBLIC CEVG_SHARED)
else()
    add_library(cevg STATIC)
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
set_target_properties(cevg_common PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)
if(MSVC)
    target_compile_options(cevg_common PRIVATE /experimental:c11atomics)
endif()

target_sources(cevg PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cevg_skia_graphite.cpp"
    $<TARGET_OBJECTS:cevg_common>
)
if(MSVC AND CEVG_SHARED)
    # Generate .def file dynamically so VK symbols are only included when CEVG_VULKAN=ON
    set(CEVG_DEF_SYMBOLS
        cevg_gpu_device_create_cpu
        cevg_gpu_device_create_auto
        cevg_gpu_device_destroy
        cevg_version
        cevg_backend_name
        cevg_context_create
        cevg_context_destroy
        cevg_context_reset
        cevg_context_tick
        cevg_context_get_vram_used
        cevg_context_get_vram_budget
        cevg_recorder_create
        cevg_recorder_destroy
        cevg_context_default_recorder
        cevg_recorder_snap
        cevg_context_insert_recording
        cevg_recording_destroy
        cevg_surface_create
        cevg_surface_create_with_recorder
        cevg_surface_create_for_buffer
        cevg_surface_resize
        cevg_surface_destroy
        cevg_surface_get_width
        cevg_surface_get_height
        cevg_surface_read_pixels
        cevg_surface_present
        cevg_surface_add_dirty_rect
        cevg_canvas_create
        cevg_canvas_create_with_recorder
        cevg_canvas_destroy
        cevg_canvas_save
        cevg_canvas_restore
        cevg_canvas_get_save_count
        cevg_canvas_translate
        cevg_canvas_scale
        cevg_canvas_rotate
        cevg_canvas_skew
        cevg_canvas_concat
        cevg_canvas_reset_matrix
        cevg_canvas_get_transform
        cevg_canvas_clip_rect
        cevg_canvas_clip_path
        cevg_canvas_get_clip_bounds
        cevg_canvas_clear
        cevg_canvas_save_layer
        cevg_canvas_restore_layer
        cevg_canvas_draw_paint
        cevg_canvas_draw_rect
        cevg_canvas_draw_round_rect
        cevg_canvas_draw_circle
        cevg_canvas_draw_oval
        cevg_canvas_draw_line
        cevg_canvas_draw_path
        cevg_canvas_draw_image
        cevg_canvas_draw_image_rect
        cevg_canvas_draw_image_nine
        cevg_canvas_draw_text_blob
        cevg_canvas_draw_surface
        cevg_canvas_flush
        cevg_display_list_record_begin
        cevg_display_list_record_end
        cevg_display_list_destroy
        cevg_canvas_draw_display_list
        cevg_path_create
        cevg_path_destroy
        cevg_path_rewind
        cevg_path_move_to
        cevg_path_line_to
        cevg_path_quad_to
        cevg_path_cubic_to
        cevg_path_close
        cevg_path_set_fill_rule
        cevg_path_get_fill_rule
        cevg_path_get_bounds
        cevg_paint_create
        cevg_paint_destroy
        cevg_paint_reset
        cevg_paint_set_color
        cevg_paint_set_color_argb
        cevg_paint_set_style
        cevg_paint_set_alpha
        cevg_paint_set_anti_alias
        cevg_paint_set_blend_mode
        cevg_paint_set_stroke_width
        cevg_paint_set_stroke_cap
        cevg_paint_set_stroke_join
        cevg_paint_set_stroke_miter
        cevg_paint_set_dash
        cevg_paint_clear_dash
        cevg_paint_set_linear_gradient
        cevg_paint_set_radial_gradient
        cevg_paint_clear_shader
        cevg_paint_set_image_shader
        cevg_paint_set_blur
        cevg_paint_set_drop_shadow
        cevg_paint_set_color_matrix
        cevg_paint_set_filter_quality
        cevg_paint_clear_filter
        cevg_paint_set_backdrop_blur
        cevg_paint_set_backdrop_shadow
        cevg_paint_clear_backdrop
        cevg_typeface_create_from_file
        cevg_typeface_create_from_data
        cevg_typeface_ref
        cevg_typeface_unref
        cevg_typeface_get_metrics
        cevg_text_blob_make
        cevg_text_blob_make_ex
        cevg_text_blob_destroy
        cevg_text_blob_get_width
        cevg_text_blob_get_height
        cevg_text_blob_get_glyph_count
        cevg_text_blob_get_glyph_positions
        cevg_text_blob_get_cluster_info
        cevg_text_blob_hit_test
        cevg_text_blob_get_glyph_advances
        cevg_text_blob_get_run_count
        cevg_text_blob_get_runs
        cevg_text_find_line_breaks
        cevg_image_create_from_file
        cevg_image_create_from_memory
        cevg_image_create_from_pixels
        cevg_image_destroy
        cevg_image_get_width
        cevg_image_get_height
    )
    if(CEVG_VULKAN)
        list(APPEND CEVG_DEF_SYMBOLS
            cevg_gpu_device_create_vulkan
            cevg_surface_create_for_vk_surface
            cevg_gpu_profiler_enable
            cevg_gpu_profiler_begin_frame
            cevg_gpu_profiler_end_frame
            cevg_gpu_profiler_get_frame_time
        )
    endif()
    # Write the .def file into the build directory
    set(CEVG_DEF_PATH "${CMAKE_CURRENT_BINARY_DIR}/cevg.def")
    file(WRITE "${CEVG_DEF_PATH}" "LIBRARY cevg\nEXPORTS\n")
    foreach(_sym ${CEVG_DEF_SYMBOLS})
        file(APPEND "${CEVG_DEF_PATH}" "    ${_sym}\n")
    endforeach()
    target_sources(cevg PRIVATE "${CEVG_DEF_PATH}")
endif()

# ---- Link Skia (built from source) ----
# Use PRIVATE so Skia's PUBLIC includes (ICU C++ headers etc.) don't
# leak into cevg_common.c which is plain C.  The C++ source file
# cevg_skia_graphite.cpp gets Skia includes via target_sources ordering.
target_link_libraries(cevg PRIVATE skia)

# Vulkan symbols are loaded at runtime by glad2 — no vulkan-1.lib needed.

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
