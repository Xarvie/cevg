# ===================================================================
# Cevg test suite
# ===================================================================
# Two test executables:
#   test_cevg_overlap  — overlap/compositing test (O001-O144)
#   test_cevg_render   — rendering test (P01-P144)
#
# The test acts as a real app: it creates Vulkan objects using raw
# PFN_vk* function pointers (no glad2 dependency) and passes them
# to cevg. cevg handles its own Vulkan function loading internally.
# ===================================================================

set(CEVG_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/stb_image_write_impl.c"
)

# ---- Common test harness sources ----
if(CEVG_VULKAN)
    set(HARNESS_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/cevg_test_harness.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/vulkan_setup.c"
    )
else()
    set(HARNESS_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/cevg_test_harness_cpu.c"
    )
endif()

# ---- Glad2 include path (for Vulkan type definitions only) ----
# The test does NOT link against cevg_glad. It only needs the
# vulkan/vulkan_core.h type definitions from the glad2 headers.
set(CEVG_GLAD_INCLUDE "${CMAKE_SOURCE_DIR}/third/glad/include")

# ---- Unified output directory (same as cevg DLL) ----
set(CEVG_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")

# ---- test_cevg_overlap ----
add_executable(test_cevg_overlap
    "${CMAKE_CURRENT_SOURCE_DIR}/test_cevg_overlap.c"
    ${HARNESS_SOURCES}
    ${CEVG_TEST_SOURCES}
)
target_include_directories(test_cevg_overlap PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CEVG_GLAD_INCLUDE}"
)
target_link_libraries(test_cevg_overlap PRIVATE cevg)
if(CEVG_VULKAN)
    target_compile_definitions(test_cevg_overlap PRIVATE CEVG_VULKAN)
endif()
set_target_properties(test_cevg_overlap PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY         "${CEVG_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CEVG_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CEVG_OUTPUT_DIR}"
)
if(MSVC)
    target_compile_options(test_cevg_overlap PRIVATE /experimental:c11atomics)
endif()

# ---- test_cevg_render ----
add_executable(test_cevg_render
    "${CMAKE_CURRENT_SOURCE_DIR}/test_cevg_render.c"
    ${HARNESS_SOURCES}
    ${CEVG_TEST_SOURCES}
)
target_include_directories(test_cevg_render PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CEVG_GLAD_INCLUDE}"
)
target_link_libraries(test_cevg_render PRIVATE cevg)
if(CEVG_VULKAN)
    target_compile_definitions(test_cevg_render PRIVATE CEVG_VULKAN)
endif()
set_target_properties(test_cevg_render PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY         "${CEVG_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CEVG_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CEVG_OUTPUT_DIR}"
)
if(MSVC)
    target_compile_options(test_cevg_render PRIVATE /experimental:c11atomics)
endif()
