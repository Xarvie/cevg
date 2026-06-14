# ============================================================================
# Skia - CMake build (top-level wrapper)
# Supports building from any official Skia source directory without modifying it.
# Usage:
#   cmake -S C:\skia -B C:\skia\build -DSKIA_SRC_DIR=skia-m147
#   cmake --build C:\skia\build --config Release
#
# Configuration options:
#   SKIA_SRC_DIR     - Skia source directory name (default: skia-m147)
#   SKIA_USE_HARFBUZZ  - Enable HarfBuzz text shaping (requires harfbuzz lib)
#   SKIA_USE_ICU       - Enable ICU unicode support (requires icu4c lib)
# ============================================================================
cmake_minimum_required(VERSION 3.16)
project(Skia VERSION 1.0.0 LANGUAGES C CXX ASM)

find_package(PkgConfig)

# ============================================================================
# Skia source directory selection (configurable via -DSKIA_SRC_DIR=xxx)
# This allows building from any official Skia clone without modifying it.
# Examples: skia-m147, skia-m150, skia
# ============================================================================
set(SKIA_SRC_DIR "skia-m150" CACHE STRING "Skia source directory name")
set(SKIA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/${SKIA_SRC_DIR}")
message(STATUS "Skia source directory: ${SKIA_ROOT}")

# ============================================================================
# Utility: filter source lists to only existing files (cross-version compat)
# ============================================================================
macro(skia_filter_sources _varname)
    set(_new "")
    foreach(_src ${${_varname}})
        if(EXISTS "${_src}")
            list(APPEND _new "${_src}")
        else()
            message(STATUS "SKIP (not found): ${_src}")
        endif()
    endforeach()
    set(${_varname} "${_new}")
endmacro()

# ============================================================================
# Options
# ============================================================================
option(SKIA_USE_HARFBUZZ "Enable HarfBuzz text shaping" ON)
option(SKIA_USE_ICU "Enable ICU unicode support" ON)
option(SKIA_USE_VULKAN "Enable Vulkan GPU backend" ON)
option(SKIA_USE_FREETYPE "Enable FreeType font rendering (cross-platform)" ON)

# ============================================================================
# Third-party dependencies
# ============================================================================
# ============================================================================
# SkiaDependencies.cmake (inlined)
# Manages third-party dependencies from local source directories.
# All deps are expected in SKIA_THIRD_DIR (default: ../third relative to Skia).
# ============================================================================

# Third-party directory (relative to Skia root)
set(SKIA_THIRD_DIR "${SKIA_ROOT}/../third" CACHE PATH "Path to third-party libraries")
get_filename_component(SKIA_THIRD_DIR "${SKIA_THIRD_DIR}" ABSOLUTE)

message(STATUS "Third-party directory: ${SKIA_THIRD_DIR}")

# ============================================================================
# ICU (custom CMakeLists.txt in third/icu)
# ============================================================================
add_subdirectory("${SKIA_THIRD_DIR}/icu" ${CMAKE_BINARY_DIR}/_deps/icu)

# Harfbuzz's CMake uses find_package(ICU), so create ICU::uc alias for icuuc
if(NOT TARGET ICU::uc)
    add_library(ICU::uc ALIAS icuuc)
endif()

# Pre-populate find_package(ICU) variables for harfbuzz
set(ICU_FOUND TRUE CACHE INTERNAL "")
set(ICU_INCLUDE_DIR "${SKIA_THIRD_DIR}/icu/source/common/unicode" CACHE INTERNAL "")
set(ICU_INCLUDE_DIRS "${ICU_INCLUDE_DIR}" CACHE INTERNAL "")
set(ICU_LIBRARY icuuc CACHE INTERNAL "")
set(ICU_LIBRARIES icuuc CACHE INTERNAL "")
set(ICU_uc_FOUND TRUE CACHE INTERNAL "")
set(ICU_VERSION 78.1 CACHE INTERNAL "")

# Also create skia_icu alias for Skia's link step
add_library(skia_icu INTERFACE)
target_link_libraries(skia_icu INTERFACE ICU::icuuc ICU::icuin)
target_include_directories(skia_icu INTERFACE
    "${SKIA_THIRD_DIR}/icu"
    "${SKIA_THIRD_DIR}/icu/source/common"
    "${SKIA_THIRD_DIR}/icu/source/common/unicode"
    "${SKIA_THIRD_DIR}/icu/source/i18n"
    "${SKIA_THIRD_DIR}/icu/source/i18n/unicode"
)

# ============================================================================
# zlib
# ============================================================================
set(BUILD_SHARED_LIBS OFF)
set(SKIP_INSTALL_ALL ON CACHE INTERNAL "Skip install for all deps")
add_subdirectory("${SKIA_THIRD_DIR}/zlib" ${CMAKE_BINARY_DIR}/_deps/zlib)
add_library(skia_zlib ALIAS zlibstatic)

# Pre-populate find_package(ZLIB) variables and create ZLIB::ZLIB target
# (required by FreeType and other deps that use find_package(ZLIB))
if(NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
endif()
set(ZLIB_FOUND TRUE CACHE INTERNAL "")
set(ZLIB_INCLUDE_DIR "${SKIA_THIRD_DIR}/zlib" "${CMAKE_BINARY_DIR}/_deps/zlib" CACHE INTERNAL "")
set(ZLIB_INCLUDE_DIRS "${ZLIB_INCLUDE_DIR}" CACHE INTERNAL "")
set(ZLIB_LIBRARIES zlibstatic CACHE INTERNAL "")
set(ZLIB_LIBRARY zlibstatic CACHE INTERNAL "")

# ============================================================================
# expat (XML parser)
# ============================================================================
# Match Skia's GN build: disable arc4random_buf/getrandom entropy sources
# that may be detected by CMake but fail with clang on some Linux distros.
# Only XML_DEV_URANDOM is used (reads from /dev/urandom), same as GN.
set(EXPAT_WITH_LIBBSD OFF CACHE BOOL "Do not link libbsd for arc4random_buf" FORCE)
set(EXPAT_SHARED_LIBS OFF CACHE BOOL "Build shared expat" FORCE)
set(EXPAT_BUILD_TESTS OFF CACHE BOOL "Build expat tests" FORCE)
set(EXPAT_BUILD_DOCS OFF CACHE BOOL "Build expat docs" FORCE)
set(EXPAT_BUILD_TOOLS OFF CACHE BOOL "Build expat tools" FORCE)
set(EXPAT_BUILD_EXAMPLES OFF CACHE BOOL "Build expat examples" FORCE)
add_subdirectory("${SKIA_THIRD_DIR}/expat/expat" ${CMAKE_BINARY_DIR}/_deps/expat)
# After add_subdirectory, override the detected entropy macros to match GN:
# - Undefine HAVE_ARC4RANDOM_BUF (detected on newer glibc but missing headers in clang)
# - Undefine HAVE_GETRANDOM / HAVE_SYSCALL_GETRANDOM (same reason)
# - Define XML_DEV_URANDOM=1 (the only entropy source Skia's GN build uses)
target_compile_definitions(expat PRIVATE
    XML_DEV_URANDOM=1
    HAVE_ARC4RANDOM_BUF=0
    HAVE_GETRANDOM=0
    HAVE_SYSCALL_GETRANDOM=0
)
add_library(skia_expat ALIAS expat)

# ============================================================================
# HarfBuzz (depends on ICU)
# ============================================================================
# Set HB_HAVE_ICU so harfbuzz compiles with ICU support
set(HB_HAVE_ICU ON CACHE BOOL "Enable ICU unicode functions" FORCE)
add_subdirectory("${SKIA_THIRD_DIR}/harfbuzz" ${CMAKE_BINARY_DIR}/_deps/harfbuzz)

# Add harfbuzz src include for consumers
target_include_directories(harfbuzz PUBLIC "${SKIA_THIRD_DIR}/harfbuzz/src")
# harfbuzz-icu also needs ICU include dirs
if(TARGET harfbuzz-icu)
    target_include_directories(harfbuzz-icu PUBLIC
        "${SKIA_THIRD_DIR}/harfbuzz/src"
        "${SKIA_THIRD_DIR}/icu/source/common"
    )
endif()

add_library(skia_harfbuzz ALIAS harfbuzz)

# ============================================================================
# FreeType (font rendering, cross-platform)
# ============================================================================
if(SKIA_USE_FREETYPE)
    # Disable: HarfBuzz (avoid HB↔FT circular dep; Skia shapes with HarfBuzz directly)
    #          Brotli   (only needed for WOFF2; apps rarely ship woff2 to FT)
    #          Bzip2    (only for bzip2-compressed PCF bitmaps; essentially unused)
    set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
    # Require: ZLIB (embedded TrueType/OpenType compression)
    set(FT_REQUIRE_ZLIB ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
    set(SKIP_INSTALL_ALL ON CACHE INTERNAL "Skip install for freetype")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build freetype static" FORCE)
    add_subdirectory("${SKIA_THIRD_DIR}/freetype" ${CMAKE_BINARY_DIR}/_deps/freetype)
    add_library(skia_freetype ALIAS freetype)
endif()

# ============================================================================
# Summary
# ============================================================================
message(STATUS "")
message(STATUS "Third-party dependencies configured:")
message(STATUS "  zlib:      ${SKIA_THIRD_DIR}/zlib")
message(STATUS "  expat:     ${SKIA_THIRD_DIR}/expat/expat")
message(STATUS "  harfbuzz:  ${SKIA_THIRD_DIR}/harfbuzz (ICU: ON)")
message(STATUS "  icu:       ${SKIA_THIRD_DIR}/icu")
if(SKIA_USE_FREETYPE)
message(STATUS "  freetype:  ${SKIA_THIRD_DIR}/freetype")
endif()
message(STATUS "")

# ============================================================================
# Compile definitions
# ============================================================================
set(SKIA_DEFINES
    SK_GAMMA_APPLY_TO_A8
    SKIA_IMPLEMENTATION=1
    SK_CODEC_DECODES_BMP
    SK_CODEC_DECODES_WBMP
    SK_ENABLE_PRECOMPILE
    SK_ENABLE_DUMP_GPU
    SK_ENABLE_AVX512_OPTS
    # SkShaper (always available)
    SK_SHAPER_PRIMITIVE_AVAILABLE
    SKSHAPER_IMPLEMENTATION=1
)

list(APPEND SKIA_DEFINES SK_GRAPHITE)

# Vulkan backend for Graphite
if(SKIA_USE_VULKAN)
    list(APPEND SKIA_DEFINES SK_VULKAN)
    # glad2 provides Vulkan headers via header hijacking — no SK_USE_INTERNAL_VULKAN_HEADERS
endif()

if(WIN32)
    list(APPEND SKIA_DEFINES SK_R32_SHIFT=16)
endif()

# ---- HarfBuzz ----
if(SKIA_USE_HARFBUZZ)
    list(APPEND SKIA_DEFINES
        SK_SHAPER_HARFBUZZ_AVAILABLE
        HAVE_OT
        HAVE_CONFIG_OVERRIDE_H
        HB_NO_FALLBACK_SHAPE
        HB_NO_WIN1256
    )
endif()

# ---- ICU / SkUnicode ----
if(SKIA_USE_ICU)
    list(APPEND SKIA_DEFINES
        SK_SHAPER_UNICODE_AVAILABLE
        SK_UNICODE_AVAILABLE
        SK_UNICODE_ICU_IMPLEMENTATION
        SKUNICODE_IMPLEMENTATION=1
        SK_USING_THIRD_PARTY_ICU
        U_STATIC_IMPLEMENTATION
        U_COMMON_IMPLEMENTATION
        U_I18N_IMPLEMENTATION
        U_ENABLE_DYLOAD=0
        U_DISABLE_RENAMING=1
        U_USING_ICU_NAMESPACE=0
        U_SHOW_CPLUSPLUS_API=0
        _XOPEN_SOURCE=0
    )
    if(WIN32)
        list(APPEND SKIA_DEFINES "U_NOEXCEPT=")
    endif()
endif()

# ---- FreeType ----
if(SKIA_USE_FREETYPE)
    list(APPEND SKIA_DEFINES
        SK_TYPEFACE_FACTORY_FREETYPE
        SK_FONTMGR_FREETYPE_EMPTY_AVAILABLE
        SK_FONTMGR_FREETYPE_DIRECTORY_AVAILABLE
        SK_FONTMGR_FREETYPE_DATA_AVAILABLE
        SK_FREETYPE_MINIMUM_RUNTIME_VERSION_IS_BUILD_VERSION=1
        FT2_BUILD_LIBRARY
        FT_CONFIG_OPTION_USE_ZLIB
        FT_CONFIG_OPTION_SVG
    )
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Wasm")
        list(APPEND SKIA_DEFINES
            "FT_CONFIG_MODULES_H=<freetype-android/freetype/config/ftmodule.h>"
            "FT_CONFIG_OPTIONS_H=<freetype-android/freetype/config/ftoption.h>"
        )
    endif()
endif()

# ---- Platform FontMgr (with full fallback support) ----
if(WIN32)
    list(APPEND SKIA_DEFINES
        SK_TYPEFACE_FACTORY_DIRECTWRITE
        SK_FONTMGR_DIRECTWRITE_AVAILABLE
    )
elseif(APPLE)
    list(APPEND SKIA_DEFINES
        SK_TYPEFACE_FACTORY_CORETEXT
        SK_FONTMGR_CORETEXT_AVAILABLE
    )
endif()

# ---- Fontconfig (Linux system font discovery) ----
if(UNIX AND NOT APPLE AND NOT ANDROID)
    if(PkgConfig_FOUND)
        pkg_check_modules(FONTCONFIG fontconfig)
        if(FONTCONFIG_FOUND)
            message(STATUS "Fontconfig found: ${FONTCONFIG_LIBRARIES}")
            list(APPEND SKIA_DEFINES SK_FONTMGR_FONTCONFIG_AVAILABLE)
        endif()
    endif()
endif()

# ============================================================================
# Include directories
# ============================================================================
set(SKIA_INCLUDES
    "${SKIA_ROOT}"
    "${SKIA_ROOT}/include"
    "${SKIA_ROOT}/include/config"
    "${SKIA_ROOT}/include/core"
    "${SKIA_ROOT}/include/codec"
    "${SKIA_ROOT}/include/effects"
    "${SKIA_ROOT}/include/encode"
    "${SKIA_ROOT}/include/gpu"
    "${SKIA_ROOT}/include/gpu/ganesh"
    "${SKIA_ROOT}/include/gpu/ganesh/gl"
    "${SKIA_ROOT}/include/pathops"
    "${SKIA_ROOT}/include/ports"
    "${SKIA_ROOT}/include/private"
    "${SKIA_ROOT}/include/private/base"
    "${SKIA_ROOT}/include/private/chromium"
    "${SKIA_ROOT}/include/sksl"
    "${SKIA_ROOT}/include/utils"
)
# glad2 provides Vulkan headers — no need for include/third_party/vulkan
list(APPEND SKIA_INCLUDES
    "${SKIA_ROOT}/modules/skshaper/include"
    "${SKIA_ROOT}/modules/skunicode/include"
    "${SKIA_ROOT}/src/core"
    "${SKIA_ROOT}/src/gpu"
    "${SKIA_ROOT}/src/gpu/ganesh"
    "${SKIA_ROOT}/src/gpu/ganesh/gl"
    "${SKIA_ROOT}/src/gpu/ganesh/gl/builders"
    "${SKIA_ROOT}/src/gpu/ganesh/effects"
    "${SKIA_ROOT}/src/gpu/ganesh/geometry"
    "${SKIA_ROOT}/src/gpu/ganesh/glsl"
    "${SKIA_ROOT}/src/gpu/ganesh/gradients"
    "${SKIA_ROOT}/src/gpu/ganesh/image"
    "${SKIA_ROOT}/src/gpu/ganesh/ops"
    "${SKIA_ROOT}/src/gpu/ganesh/surface"
    "${SKIA_ROOT}/src/gpu/ganesh/tessellate"
    "${SKIA_ROOT}/src/gpu/ganesh/text"
    "${SKIA_ROOT}/src/gpu/tessellate"
    "${SKIA_ROOT}/src/sfnt"
    "${SKIA_ROOT}/src/sksl"
    "${SKIA_ROOT}/src/text"
    "${SKIA_ROOT}/src/text/gpu"
    "${SKIA_ROOT}/src/utils"
    "${SKIA_ROOT}/third_party"
)

list(APPEND SKIA_INCLUDES
        "${SKIA_ROOT}/include/gpu/graphite"
        "${SKIA_ROOT}/src/gpu/graphite"
    )

# ============================================================================
# Source files - Core library
# ============================================================================

# --- Core sources ---
    set(SKIA_CORE_HEADERS
        "${SKIA_ROOT}/include/private/SkGainmapInfo.h"
        "${SKIA_ROOT}/include/private/SkGainmapShader.h"
        "${SKIA_ROOT}/include/private/SkHdrMetadata.h"
        "${SKIA_ROOT}/include/private/SkIDChangeListener.h"
        "${SKIA_ROOT}/include/private/SkPixelStorage.h"
        "${SKIA_ROOT}/include/private/SkSLSampleUsage.h"
        "${SKIA_ROOT}/include/private/base/SingleOwner.h"
        "${SKIA_ROOT}/include/private/base/SkAPI.h"
        "${SKIA_ROOT}/include/private/base/SkASAN.h"
        "${SKIA_ROOT}/include/private/base/SkAlign.h"
        "${SKIA_ROOT}/include/private/base/SkAlignedStorage.h"
        "${SKIA_ROOT}/include/private/base/SkAnySubclass.h"
        "${SKIA_ROOT}/include/private/base/SkAssert.h"
        "${SKIA_ROOT}/include/private/base/SkAttributes.h"
        "${SKIA_ROOT}/include/private/base/SkCPUTypes.h"
        "${SKIA_ROOT}/include/private/base/SkContainers.h"
        "${SKIA_ROOT}/include/private/base/SkDebug.h"
        "${SKIA_ROOT}/include/private/base/SkDeque.h"
        "${SKIA_ROOT}/include/private/base/SkFeatures.h"
        "${SKIA_ROOT}/include/private/base/SkFixed.h"
        "${SKIA_ROOT}/include/private/base/SkFloatingPoint.h"
        "${SKIA_ROOT}/include/private/base/SkLoadUserConfig.h"
        "${SKIA_ROOT}/include/private/base/SkLog.h"
        "${SKIA_ROOT}/include/private/base/SkLogPriority.h"
        "${SKIA_ROOT}/include/private/base/SkMacros.h"
        "${SKIA_ROOT}/include/private/base/SkMalloc.h"
        "${SKIA_ROOT}/include/private/base/SkMath.h"
        "${SKIA_ROOT}/include/private/base/SkMutex.h"
        "${SKIA_ROOT}/include/private/base/SkNoncopyable.h"
        "${SKIA_ROOT}/include/private/base/SkOnce.h"
        "${SKIA_ROOT}/include/private/base/SkPoint_impl.h"
        "${SKIA_ROOT}/include/private/base/SkSafe32.h"
        "${SKIA_ROOT}/include/private/base/SkSemaphore.h"
        "${SKIA_ROOT}/include/private/base/SkSpan_impl.h"
        "${SKIA_ROOT}/include/private/base/SkTArray.h"
        "${SKIA_ROOT}/include/private/base/SkTDArray.h"
        "${SKIA_ROOT}/include/private/base/SkTFitsIn.h"
        "${SKIA_ROOT}/include/private/base/SkTLogic.h"
        "${SKIA_ROOT}/include/private/base/SkTPin.h"
        "${SKIA_ROOT}/include/private/base/SkTemplates.h"
        "${SKIA_ROOT}/include/private/base/SkThreadAnnotations.h"
        "${SKIA_ROOT}/include/private/base/SkThreadID.h"
        "${SKIA_ROOT}/include/private/base/SkTo.h"
        "${SKIA_ROOT}/include/private/base/SkTypeTraits.h"
        "${SKIA_ROOT}/include/private/chromium/SkChromeRemoteGlyphCache.h"
        "${SKIA_ROOT}/include/private/chromium/SkDiscardableMemory.h"
        "${SKIA_ROOT}/include/private/chromium/SkPMColor.h"
        "${SKIA_ROOT}/include/private/chromium/Slug.h"
        "${SKIA_ROOT}/src/base/SkArenaAlloc.h"
        "${SKIA_ROOT}/src/base/SkArenaAllocList.h"
        "${SKIA_ROOT}/src/base/SkAutoLocaleSetter.h"
        "${SKIA_ROOT}/src/base/SkAutoMalloc.h"
        "${SKIA_ROOT}/src/base/SkBase64.h"
        "${SKIA_ROOT}/src/base/SkBezierCurves.h"
        "${SKIA_ROOT}/src/base/SkBitmaskEnum.h"
        "${SKIA_ROOT}/src/base/SkBlockAllocator.h"
        "${SKIA_ROOT}/src/base/SkBuffer.h"
        "${SKIA_ROOT}/src/base/SkCubics.h"
        "${SKIA_ROOT}/src/base/SkEndian.h"
        "${SKIA_ROOT}/src/base/SkEnumBitMask.h"
        "${SKIA_ROOT}/src/base/SkFixedArray.h"
        "${SKIA_ROOT}/src/base/SkFloatBits.h"
        "${SKIA_ROOT}/src/base/SkHalf.h"
        "${SKIA_ROOT}/src/base/SkLeanWindows.h"
        "${SKIA_ROOT}/src/base/SkMSAN.h"
        "${SKIA_ROOT}/src/base/SkMathPriv.h"
        "${SKIA_ROOT}/src/base/SkNoDestructor.h"
        "${SKIA_ROOT}/src/base/SkQuads.h"
        "${SKIA_ROOT}/src/base/SkRandom.h"
        "${SKIA_ROOT}/src/base/SkRectMemcpy.h"
        "${SKIA_ROOT}/src/base/SkSafeMath.h"
        "${SKIA_ROOT}/src/base/SkScopeExit.h"
        "${SKIA_ROOT}/src/base/SkSharedMutex.h"
        "${SKIA_ROOT}/src/base/SkSpinlock.h"
        "${SKIA_ROOT}/src/base/SkStringView.h"
        "${SKIA_ROOT}/src/base/SkTBlockList.h"
        "${SKIA_ROOT}/src/base/SkTDPQueue.h"
        "${SKIA_ROOT}/src/base/SkTInternalLList.h"
        "${SKIA_ROOT}/src/base/SkTLazy.h"
        "${SKIA_ROOT}/src/base/SkTSearch.h"
        "${SKIA_ROOT}/src/base/SkTSort.h"
        "${SKIA_ROOT}/src/base/SkTime.h"
        "${SKIA_ROOT}/src/base/SkUTF.h"
        "${SKIA_ROOT}/src/base/SkUtils.h"
        "${SKIA_ROOT}/src/base/SkVx.h"
        "${SKIA_ROOT}/src/base/SkZip.h"
        "${SKIA_ROOT}/src/capture/SkCapture.h"
        "${SKIA_ROOT}/src/capture/SkCaptureCanvas.h"
        "${SKIA_ROOT}/src/capture/SkCaptureManager.h"
        "${SKIA_ROOT}/src/core/Sk4px.h"
        "${SKIA_ROOT}/src/core/SkAAClip.h"
        "${SKIA_ROOT}/src/core/SkATrace.h"
        "${SKIA_ROOT}/src/core/SkAdvancedTypefaceMetrics.h"
        "${SKIA_ROOT}/src/core/SkAlphaRuns.h"
        "${SKIA_ROOT}/src/core/SkAnalyticEdge.h"
        "${SKIA_ROOT}/src/core/SkAnnotationKeys.h"
        "${SKIA_ROOT}/src/core/SkAutoBlitterChoose.h"
        "${SKIA_ROOT}/src/core/SkAutoPixmapStorage.h"
        "${SKIA_ROOT}/src/core/SkBigPicture.h"
        "${SKIA_ROOT}/src/core/SkBitmapCache.h"
        "${SKIA_ROOT}/src/core/SkBitmapDevice.h"
        "${SKIA_ROOT}/src/core/SkBitmapProcState.h"
        "${SKIA_ROOT}/src/core/SkBlendModeBlender.h"
        "${SKIA_ROOT}/src/core/SkBlendModePriv.h"
        "${SKIA_ROOT}/src/core/SkBlenderBase.h"
        "${SKIA_ROOT}/src/core/SkBlitBWMaskTemplate.h"
        "${SKIA_ROOT}/src/core/SkBlitMask.h"
        "${SKIA_ROOT}/src/core/SkBlitRow.h"
        "${SKIA_ROOT}/src/core/SkBlitter.h"
        "${SKIA_ROOT}/src/core/SkBlitter_A8.h"
        "${SKIA_ROOT}/src/core/SkBlurEngine.h"
        "${SKIA_ROOT}/src/core/SkBlurMask.h"
        "${SKIA_ROOT}/src/core/SkBlurMaskFilterImpl.h"
        "${SKIA_ROOT}/src/core/SkCPUContextImpl.h"
        "${SKIA_ROOT}/src/core/SkCPURecorderImpl.h"
        "${SKIA_ROOT}/src/core/SkCachedData.h"
        "${SKIA_ROOT}/src/core/SkCanvasPriv.h"
        "${SKIA_ROOT}/src/core/SkChecksum.h"
        "${SKIA_ROOT}/src/core/SkClipStack.h"
        "${SKIA_ROOT}/src/core/SkClipStackDevice.h"
        "${SKIA_ROOT}/src/core/SkColorData.h"
        "${SKIA_ROOT}/src/core/SkColorFilterPriv.h"
        "${SKIA_ROOT}/src/core/SkColorPriv.h"
        "${SKIA_ROOT}/src/core/SkColorSpacePriv.h"
        "${SKIA_ROOT}/src/core/SkColorSpaceXformSteps.h"
        "${SKIA_ROOT}/src/core/SkCompressedDataUtils.h"
        "${SKIA_ROOT}/src/core/SkConvertPixels.h"
        "${SKIA_ROOT}/src/core/SkCoreBlitters.h"
        "${SKIA_ROOT}/src/core/SkCpu.h"
        "${SKIA_ROOT}/src/core/SkCubicClipper.h"
        "${SKIA_ROOT}/src/core/SkDebugUtils.h"
        "${SKIA_ROOT}/src/core/SkDescriptor.h"
        "${SKIA_ROOT}/src/core/SkDevice.h"
        "${SKIA_ROOT}/src/core/SkDistanceFieldGen.h"
        "${SKIA_ROOT}/src/core/SkDraw.h"
        "${SKIA_ROOT}/src/core/SkDrawProcs.h"
        "${SKIA_ROOT}/src/core/SkDrawShadowInfo.h"
        "${SKIA_ROOT}/src/core/SkDrawTypes.h"
        "${SKIA_ROOT}/src/core/SkEdge.h"
        "${SKIA_ROOT}/src/core/SkEdgeBuilder.h"
        "${SKIA_ROOT}/src/core/SkEdgeClipper.h"
        "${SKIA_ROOT}/src/core/SkEffectPriv.h"
        "${SKIA_ROOT}/src/core/SkEnumerate.h"
        "${SKIA_ROOT}/src/core/SkFDot6.h"
        "${SKIA_ROOT}/src/core/SkFontDescriptor.h"
        "${SKIA_ROOT}/src/core/SkFontMetricsPriv.h"
        "${SKIA_ROOT}/src/core/SkFontPriv.h"
        "${SKIA_ROOT}/src/core/SkFontStream.h"
        "${SKIA_ROOT}/src/core/SkGaussFilter.h"
        "${SKIA_ROOT}/src/core/SkGeometry.h"
        "${SKIA_ROOT}/src/core/SkGlyph.h"
        "${SKIA_ROOT}/src/core/SkGlyphRunPainter.h"
        "${SKIA_ROOT}/src/core/SkIPoint16.h"
        "${SKIA_ROOT}/src/core/SkImageFilterCache.h"
        "${SKIA_ROOT}/src/core/SkImageFilterTypes.h"
        "${SKIA_ROOT}/src/core/SkImageFilter_Base.h"
        "${SKIA_ROOT}/src/core/SkImageInfoPriv.h"
        "${SKIA_ROOT}/src/core/SkKnownRuntimeEffects.h"
        "${SKIA_ROOT}/src/core/SkLRUCache.h"
        "${SKIA_ROOT}/src/core/SkLatticeIter.h"
        "${SKIA_ROOT}/src/core/SkLineClipper.h"
        "${SKIA_ROOT}/src/core/SkLocalMatrixImageFilter.h"
        "${SKIA_ROOT}/src/core/SkMD5.h"
        "${SKIA_ROOT}/src/core/SkMask.h"
        "${SKIA_ROOT}/src/core/SkMaskBlurFilter.h"
        "${SKIA_ROOT}/src/core/SkMaskCache.h"
        "${SKIA_ROOT}/src/core/SkMaskFilterBase.h"
        "${SKIA_ROOT}/src/core/SkMaskGamma.h"
        "${SKIA_ROOT}/src/core/SkMasks.h"
        "${SKIA_ROOT}/src/core/SkMatrixInvert.h"
        "${SKIA_ROOT}/src/core/SkMatrixPriv.h"
        "${SKIA_ROOT}/src/core/SkMatrixUtils.h"
        "${SKIA_ROOT}/src/core/SkMemset.h"
        "${SKIA_ROOT}/src/core/SkMeshPriv.h"
        "${SKIA_ROOT}/src/core/SkMessageBus.h"
        "${SKIA_ROOT}/src/core/SkMipmap.h"
        "${SKIA_ROOT}/src/core/SkMipmapAccessor.h"
        "${SKIA_ROOT}/src/core/SkMipmapBuilder.h"
        "${SKIA_ROOT}/src/core/SkNextID.h"
        "${SKIA_ROOT}/src/core/SkOSFile.h"
        "${SKIA_ROOT}/src/core/SkOpts.h"
        "${SKIA_ROOT}/src/core/SkOptsTargets.h"
        "${SKIA_ROOT}/src/core/SkPaintDefaults.h"
        "${SKIA_ROOT}/src/core/SkPaintPriv.h"
        "${SKIA_ROOT}/src/core/SkPathData.h"
        "${SKIA_ROOT}/src/core/SkPathEffectBase.h"
        "${SKIA_ROOT}/src/core/SkPathEnums.h"
        "${SKIA_ROOT}/src/core/SkPathMakers.h"
        "${SKIA_ROOT}/src/core/SkPathMeasurePriv.h"
        "${SKIA_ROOT}/src/core/SkPathPriv.h"
        "${SKIA_ROOT}/src/core/SkPathRaw.h"
        "${SKIA_ROOT}/src/core/SkPathRawShapes.h"
        "${SKIA_ROOT}/src/core/SkPictureData.h"
        "${SKIA_ROOT}/src/core/SkPictureFlat.h"
        "${SKIA_ROOT}/src/core/SkPicturePlayback.h"
        "${SKIA_ROOT}/src/core/SkPicturePriv.h"
        "${SKIA_ROOT}/src/core/SkPictureRecord.h"
        "${SKIA_ROOT}/src/core/SkPixelRefPriv.h"
        "${SKIA_ROOT}/src/core/SkPointPriv.h"
        "${SKIA_ROOT}/src/core/SkPtrRecorder.h"
        "${SKIA_ROOT}/src/core/SkQuadClipper.h"
        "${SKIA_ROOT}/src/core/SkRRectPriv.h"
        "${SKIA_ROOT}/src/core/SkRTree.h"
        "${SKIA_ROOT}/src/core/SkRasterClip.h"
        "${SKIA_ROOT}/src/core/SkRasterClipStack.h"
        "${SKIA_ROOT}/src/core/SkRasterPipeline.h"
        "${SKIA_ROOT}/src/core/SkRasterPipelineContextUtils.h"
        "${SKIA_ROOT}/src/core/SkRasterPipelineOpContexts.h"
        "${SKIA_ROOT}/src/core/SkRasterPipelineOpList.h"
        "${SKIA_ROOT}/src/core/SkRasterPipelineVizualizer.h"
        "${SKIA_ROOT}/src/core/SkReadBuffer.h"
        "${SKIA_ROOT}/src/core/SkReadPixelsRec.h"
        "${SKIA_ROOT}/src/core/SkRecord.h"
        "${SKIA_ROOT}/src/core/SkRecordCanvas.h"
        "${SKIA_ROOT}/src/core/SkRecordDraw.h"
        "${SKIA_ROOT}/src/core/SkRecordOpts.h"
        "${SKIA_ROOT}/src/core/SkRecordPattern.h"
        "${SKIA_ROOT}/src/core/SkRecordedDrawable.h"
        "${SKIA_ROOT}/src/core/SkRecords.h"
        "${SKIA_ROOT}/src/core/SkRectPriv.h"
        "${SKIA_ROOT}/src/core/SkRegionPriv.h"
        "${SKIA_ROOT}/src/core/SkResourceCache.h"
        "${SKIA_ROOT}/src/core/SkRuntimeBlender.h"
        "${SKIA_ROOT}/src/core/SkRuntimeEffectPriv.h"
        "${SKIA_ROOT}/src/core/SkSLTypeShared.h"
        "${SKIA_ROOT}/src/core/SkSafeRange.h"
        "${SKIA_ROOT}/src/core/SkSamplingPriv.h"
        "${SKIA_ROOT}/src/core/SkScaleToSides.h"
        "${SKIA_ROOT}/src/core/SkScalerContext.h"
        "${SKIA_ROOT}/src/core/SkScan.h"
        "${SKIA_ROOT}/src/core/SkScanPriv.h"
        "${SKIA_ROOT}/src/core/SkSpanPriv.h"
        "${SKIA_ROOT}/src/core/SkSpecialImage.h"
        "${SKIA_ROOT}/src/core/SkSpriteBlitter.h"
        "${SKIA_ROOT}/src/core/SkStreamPriv.h"
        "${SKIA_ROOT}/src/core/SkStrike.h"
        "${SKIA_ROOT}/src/core/SkStrikeCache.h"
        "${SKIA_ROOT}/src/core/SkStrikeSpec.h"
        "${SKIA_ROOT}/src/core/SkStringUtils.h"
        "${SKIA_ROOT}/src/core/SkStroke.h"
        "${SKIA_ROOT}/src/core/SkStrokerPriv.h"
        "${SKIA_ROOT}/src/core/SkSurfacePriv.h"
        "${SKIA_ROOT}/src/core/SkSwizzlePriv.h"
        "${SKIA_ROOT}/src/core/SkSynchronizedResourceCache.h"
        "${SKIA_ROOT}/src/core/SkTDynamicHash.h"
        "${SKIA_ROOT}/src/core/SkTHash.h"
        "${SKIA_ROOT}/src/core/SkTMultiMap.h"
        "${SKIA_ROOT}/src/core/SkTaskGroup.h"
        "${SKIA_ROOT}/src/core/SkTextBlobPriv.h"
        "${SKIA_ROOT}/src/core/SkTextFormatParams.h"
        "${SKIA_ROOT}/src/core/SkTraceEvent.h"
        "${SKIA_ROOT}/src/core/SkTraceEventCommon.h"
        "${SKIA_ROOT}/src/core/SkTypefaceCache.h"
        "${SKIA_ROOT}/src/core/SkTypeface_remote.h"
        "${SKIA_ROOT}/src/core/SkValidationUtils.h"
        "${SKIA_ROOT}/src/core/SkVertState.h"
        "${SKIA_ROOT}/src/core/SkVerticesPriv.h"
        "${SKIA_ROOT}/src/core/SkWriteBuffer.h"
        "${SKIA_ROOT}/src/core/SkWritePixelsRec.h"
        "${SKIA_ROOT}/src/core/SkWriter32.h"
        "${SKIA_ROOT}/src/core/SkYUVAInfoLocation.h"
        "${SKIA_ROOT}/src/core/SkYUVMath.h"
        "${SKIA_ROOT}/src/core/SkYUVPlanesCache.h"
        "${SKIA_ROOT}/src/image/SkImageGeneratorPriv.h"
        "${SKIA_ROOT}/src/image/SkImage_Base.h"
        "${SKIA_ROOT}/src/image/SkImage_Lazy.h"
        "${SKIA_ROOT}/src/image/SkImage_Picture.h"
        "${SKIA_ROOT}/src/image/SkImage_Raster.h"
        "${SKIA_ROOT}/src/image/SkPictureImageGenerator.h"
        "${SKIA_ROOT}/src/image/SkRescaleAndReadPixels.h"
        "${SKIA_ROOT}/src/image/SkSurface_Base.h"
        "${SKIA_ROOT}/src/image/SkSurface_Raster.h"
        "${SKIA_ROOT}/src/lazy/SkDiscardableMemoryPool.h"
        "${SKIA_ROOT}/src/opts/SkBitmapProcState_opts.h"
        "${SKIA_ROOT}/src/opts/SkBlitMask_opts.h"
        "${SKIA_ROOT}/src/opts/SkBlitRow_opts.h"
        "${SKIA_ROOT}/src/opts/SkMemset_opts.h"
        "${SKIA_ROOT}/src/opts/SkOpts_RestoreTarget.h"
        "${SKIA_ROOT}/src/opts/SkOpts_SetTarget.h"
        "${SKIA_ROOT}/src/opts/SkRasterPipeline_opts.h"
        "${SKIA_ROOT}/src/shaders/SkBitmapProcShader.h"
        "${SKIA_ROOT}/src/shaders/SkBlendShader.h"
        "${SKIA_ROOT}/src/shaders/SkColorFilterShader.h"
        "${SKIA_ROOT}/src/shaders/SkColorShader.h"
        "${SKIA_ROOT}/src/shaders/SkCoordClampShader.h"
        "${SKIA_ROOT}/src/shaders/SkEmptyShader.h"
        "${SKIA_ROOT}/src/shaders/SkImageShader.h"
        "${SKIA_ROOT}/src/shaders/SkLocalMatrixShader.h"
        "${SKIA_ROOT}/src/shaders/SkPerlinNoiseShaderImpl.h"
        "${SKIA_ROOT}/src/shaders/SkPerlinNoiseShaderType.h"
        "${SKIA_ROOT}/src/shaders/SkPictureShader.h"
        "${SKIA_ROOT}/src/shaders/SkRuntimeShader.h"
        "${SKIA_ROOT}/src/shaders/SkShaderBase.h"
        "${SKIA_ROOT}/src/shaders/SkTransformShader.h"
        "${SKIA_ROOT}/src/shaders/SkTriColorShader.h"
        "${SKIA_ROOT}/src/shaders/SkWorkingColorSpaceShader.h"
        "${SKIA_ROOT}/src/text/GlyphRun.h"
        "${SKIA_ROOT}/src/text/StrikeForGPU.h"
    )

    set(SKIA_CORE_SOURCES
        "${SKIA_ROOT}/src/base/SkArenaAlloc.cpp"
        "${SKIA_ROOT}/src/base/SkBase64.cpp"
        "${SKIA_ROOT}/src/base/SkBezierCurves.cpp"
        "${SKIA_ROOT}/src/base/SkBlockAllocator.cpp"
        "${SKIA_ROOT}/src/base/SkBuffer.cpp"
        "${SKIA_ROOT}/src/base/SkContainers.cpp"
        "${SKIA_ROOT}/src/base/SkCubics.cpp"
        "${SKIA_ROOT}/src/base/SkDebug.cpp"
        "${SKIA_ROOT}/src/base/SkDeque.cpp"
        "${SKIA_ROOT}/src/base/SkFloatingPoint.cpp"
        "${SKIA_ROOT}/src/base/SkHalf.cpp"
        "${SKIA_ROOT}/src/base/SkLog.cpp"
        "${SKIA_ROOT}/src/base/SkMalloc.cpp"
        "${SKIA_ROOT}/src/base/SkMathPriv.cpp"
        "${SKIA_ROOT}/src/base/SkQuads.cpp"
        "${SKIA_ROOT}/src/base/SkSafeMath.cpp"
        "${SKIA_ROOT}/src/base/SkSemaphore.cpp"
        "${SKIA_ROOT}/src/base/SkSharedMutex.cpp"
        "${SKIA_ROOT}/src/base/SkSpinlock.cpp"
        "${SKIA_ROOT}/src/base/SkTDArray.cpp"
        "${SKIA_ROOT}/src/base/SkTSearch.cpp"
        "${SKIA_ROOT}/src/base/SkThreadID.cpp"
        "${SKIA_ROOT}/src/base/SkTime.cpp"
        "${SKIA_ROOT}/src/base/SkUTF.cpp"
        "${SKIA_ROOT}/src/base/SkUtils.cpp"
        "${SKIA_ROOT}/src/capture/SkCapture.cpp"
        "${SKIA_ROOT}/src/capture/SkCaptureCanvas.cpp"
        "${SKIA_ROOT}/src/capture/SkCaptureManager.cpp"
        "${SKIA_ROOT}/src/core/SkAAClip.cpp"
        "${SKIA_ROOT}/src/core/SkATrace.cpp"
        "${SKIA_ROOT}/src/core/SkAlphaRuns.cpp"
        "${SKIA_ROOT}/src/core/SkAnalyticEdge.cpp"
        "${SKIA_ROOT}/src/core/SkAnnotation.cpp"
        "${SKIA_ROOT}/src/core/SkAutoPixmapStorage.cpp"
        "${SKIA_ROOT}/src/core/SkBBHFactory.cpp"
        "${SKIA_ROOT}/src/core/SkBigPicture.cpp"
        "${SKIA_ROOT}/src/core/SkBitmap.cpp"
        "${SKIA_ROOT}/src/core/SkBitmapCache.cpp"
        "${SKIA_ROOT}/src/core/SkBitmapDevice.cpp"
        "${SKIA_ROOT}/src/core/SkBitmapProcState.cpp"
        "${SKIA_ROOT}/src/core/SkBitmapProcState_matrixProcs.cpp"
        "${SKIA_ROOT}/src/core/SkBitmapProcState_opts.cpp"
        "${SKIA_ROOT}/src/core/SkBitmapProcState_opts_lasx.cpp"
        "${SKIA_ROOT}/src/core/SkBitmapProcState_opts_ssse3.cpp"
        "${SKIA_ROOT}/src/core/SkBlendMode.cpp"
        "${SKIA_ROOT}/src/core/SkBlendModeBlender.cpp"
        "${SKIA_ROOT}/src/core/SkBlitMask_opts.cpp"
        "${SKIA_ROOT}/src/core/SkBlitMask_opts_ssse3.cpp"
        "${SKIA_ROOT}/src/core/SkBlitRow_D32.cpp"
        "${SKIA_ROOT}/src/core/SkBlitRow_opts.cpp"
        "${SKIA_ROOT}/src/core/SkBlitRow_opts_lasx.cpp"
        "${SKIA_ROOT}/src/core/SkBlitRow_opts_ml3.cpp"
        "${SKIA_ROOT}/src/core/SkBlitter.cpp"
        "${SKIA_ROOT}/src/core/SkBlitter_A8.cpp"
        "${SKIA_ROOT}/src/core/SkBlitter_ARGB32.cpp"
        "${SKIA_ROOT}/src/core/SkBlitter_Sprite.cpp"
        "${SKIA_ROOT}/src/core/SkBlurEngine.cpp"
        "${SKIA_ROOT}/src/core/SkBlurMask.cpp"
        "${SKIA_ROOT}/src/core/SkBlurMaskFilterImpl.cpp"
        "${SKIA_ROOT}/src/core/SkCPUContext.cpp"
        "${SKIA_ROOT}/src/core/SkCPURecorder.cpp"
        "${SKIA_ROOT}/src/core/SkCachedData.cpp"
        "${SKIA_ROOT}/src/core/SkCanvas.cpp"
        "${SKIA_ROOT}/src/core/SkCanvasPriv.cpp"
        "${SKIA_ROOT}/src/core/SkCanvas_Raster.cpp"
        "${SKIA_ROOT}/src/core/SkCapabilities.cpp"
        "${SKIA_ROOT}/src/core/SkChecksum.cpp"
        "${SKIA_ROOT}/src/core/SkClipStack.cpp"
        "${SKIA_ROOT}/src/core/SkClipStackDevice.cpp"
        "${SKIA_ROOT}/src/core/SkColor.cpp"
        "${SKIA_ROOT}/src/core/SkColorFilter.cpp"
        "${SKIA_ROOT}/src/core/SkColorSpace.cpp"
        "${SKIA_ROOT}/src/core/SkColorSpaceXformSteps.cpp"
        "${SKIA_ROOT}/src/core/SkColorTable.cpp"
        "${SKIA_ROOT}/src/core/SkCompressedDataUtils.cpp"
        "${SKIA_ROOT}/src/core/SkContourMeasure.cpp"
        "${SKIA_ROOT}/src/core/SkConvertPixels.cpp"
        "${SKIA_ROOT}/src/core/SkCpu.cpp"
        "${SKIA_ROOT}/src/core/SkCubicClipper.cpp"
        "${SKIA_ROOT}/src/core/SkCubicMap.cpp"
        "${SKIA_ROOT}/src/core/SkData.cpp"
        "${SKIA_ROOT}/src/core/SkDataTable.cpp"
        "${SKIA_ROOT}/src/core/SkDescriptor.cpp"
        "${SKIA_ROOT}/src/core/SkDevice.cpp"
        "${SKIA_ROOT}/src/core/SkDistanceFieldGen.cpp"
        "${SKIA_ROOT}/src/core/SkDocument.cpp"
        "${SKIA_ROOT}/src/core/SkDraw.cpp"
        "${SKIA_ROOT}/src/core/SkDrawShadowInfo.cpp"
        "${SKIA_ROOT}/src/core/SkDraw_atlas.cpp"
        "${SKIA_ROOT}/src/core/SkDraw_text.cpp"
        "${SKIA_ROOT}/src/core/SkDraw_vertices.cpp"
        "${SKIA_ROOT}/src/core/SkDrawable.cpp"
        "${SKIA_ROOT}/src/core/SkEdge.cpp"
        "${SKIA_ROOT}/src/core/SkEdgeBuilder.cpp"
        "${SKIA_ROOT}/src/core/SkEdgeClipper.cpp"
        "${SKIA_ROOT}/src/core/SkExecutor.cpp"
        "${SKIA_ROOT}/src/core/SkFlattenable.cpp"
        "${SKIA_ROOT}/src/core/SkFont.cpp"
        "${SKIA_ROOT}/src/core/SkFontDescriptor.cpp"
        "${SKIA_ROOT}/src/core/SkFontMetricsPriv.cpp"
        "${SKIA_ROOT}/src/core/SkFontMgr.cpp"
        "${SKIA_ROOT}/src/core/SkFontStream.cpp"
        "${SKIA_ROOT}/src/core/SkFont_serial.cpp"
        "${SKIA_ROOT}/src/core/SkGaussFilter.cpp"
        "${SKIA_ROOT}/src/core/SkGeometry.cpp"
        "${SKIA_ROOT}/src/core/SkGlobalInitialization_core.cpp"
        "${SKIA_ROOT}/src/core/SkGlyph.cpp"
        "${SKIA_ROOT}/src/core/SkGlyphRunPainter.cpp"
        "${SKIA_ROOT}/src/core/SkGraphics.cpp"
        "${SKIA_ROOT}/src/core/SkIDChangeListener.cpp"
        "${SKIA_ROOT}/src/core/SkImageFilter.cpp"
        "${SKIA_ROOT}/src/core/SkImageFilterCache.cpp"
        "${SKIA_ROOT}/src/core/SkImageFilterTypes.cpp"
        "${SKIA_ROOT}/src/core/SkImageGenerator.cpp"
        "${SKIA_ROOT}/src/core/SkImageInfo.cpp"
        "${SKIA_ROOT}/src/core/SkKnownRuntimeEffects.cpp"
        "${SKIA_ROOT}/src/core/SkLatticeIter.cpp"
        "${SKIA_ROOT}/src/core/SkLineClipper.cpp"
        "${SKIA_ROOT}/src/core/SkLocalMatrixImageFilter.cpp"
        "${SKIA_ROOT}/src/core/SkM44.cpp"
        "${SKIA_ROOT}/src/core/SkMD5.cpp"
        "${SKIA_ROOT}/src/core/SkMallocPixelRef.cpp"
        "${SKIA_ROOT}/src/core/SkMask.cpp"
        "${SKIA_ROOT}/src/core/SkMaskBlurFilter.cpp"
        "${SKIA_ROOT}/src/core/SkMaskCache.cpp"
        "${SKIA_ROOT}/src/core/SkMaskFilter.cpp"
        "${SKIA_ROOT}/src/core/SkMaskFilterBase.cpp"
        "${SKIA_ROOT}/src/core/SkMaskGamma.cpp"
        "${SKIA_ROOT}/src/core/SkMasks.cpp"
        "${SKIA_ROOT}/src/core/SkMatrix.cpp"
        "${SKIA_ROOT}/src/core/SkMatrixInvert.cpp"
        "${SKIA_ROOT}/src/core/SkMemset_opts.cpp"
        "${SKIA_ROOT}/src/core/SkMemset_opts_avx.cpp"
        "${SKIA_ROOT}/src/core/SkMemset_opts_erms.cpp"
        "${SKIA_ROOT}/src/core/SkMesh.cpp"
        "${SKIA_ROOT}/src/core/SkMipmap.cpp"
        "${SKIA_ROOT}/src/core/SkMipmapAccessor.cpp"
        "${SKIA_ROOT}/src/core/SkMipmapBuilder.cpp"
        "${SKIA_ROOT}/src/core/SkMipmapDrawDownSampler.cpp"
        "${SKIA_ROOT}/src/core/SkMipmapHQDownSampler.cpp"
        "${SKIA_ROOT}/src/core/SkOpts.cpp"
        "${SKIA_ROOT}/src/opts/SkOpts_ml3.cpp"
        "${SKIA_ROOT}/src/opts/SkOpts_ml4.cpp"
        "${SKIA_ROOT}/src/core/SkOverdrawCanvas.cpp"
        "${SKIA_ROOT}/src/core/SkPaint.cpp"
        "${SKIA_ROOT}/src/core/SkPaintPriv.cpp"
        "${SKIA_ROOT}/src/core/SkPath.cpp"
        "${SKIA_ROOT}/src/core/SkPathBuilder.cpp"
        "${SKIA_ROOT}/src/core/SkPathData.cpp"
        "${SKIA_ROOT}/src/core/SkPathDump.cpp"
        "${SKIA_ROOT}/src/core/SkPathEffect.cpp"
        "${SKIA_ROOT}/src/core/SkPathIter.cpp"
        "${SKIA_ROOT}/src/core/SkPathMeasure.cpp"
        "${SKIA_ROOT}/src/core/SkPathPriv.cpp"
        "${SKIA_ROOT}/src/core/SkPathRaw.cpp"
        "${SKIA_ROOT}/src/core/SkPathRawShapes.cpp"
        "${SKIA_ROOT}/src/core/SkPathUtils.cpp"
        "${SKIA_ROOT}/src/core/SkPath_interpolate.cpp"
        "${SKIA_ROOT}/src/core/SkPath_serial.cpp"
        "${SKIA_ROOT}/src/core/SkPicture.cpp"
        "${SKIA_ROOT}/src/core/SkPictureData.cpp"
        "${SKIA_ROOT}/src/core/SkPictureFlat.cpp"
        "${SKIA_ROOT}/src/core/SkPicturePlayback.cpp"
        "${SKIA_ROOT}/src/core/SkPictureRecord.cpp"
        "${SKIA_ROOT}/src/core/SkPictureRecorder.cpp"
        "${SKIA_ROOT}/src/core/SkPixelRef.cpp"
        "${SKIA_ROOT}/src/core/SkPixelStorage.cpp"
        "${SKIA_ROOT}/src/core/SkPixmap.cpp"
        "${SKIA_ROOT}/src/core/SkPixmapDraw.cpp"
        "${SKIA_ROOT}/src/core/SkPoint.cpp"
        "${SKIA_ROOT}/src/core/SkPoint3.cpp"
        "${SKIA_ROOT}/src/core/SkPtrRecorder.cpp"
        "${SKIA_ROOT}/src/core/SkQuadClipper.cpp"
        "${SKIA_ROOT}/src/core/SkRRect.cpp"
        "${SKIA_ROOT}/src/core/SkRSXform.cpp"
        "${SKIA_ROOT}/src/core/SkRTree.cpp"
        "${SKIA_ROOT}/src/core/SkRasterClip.cpp"
        "${SKIA_ROOT}/src/core/SkRasterPipeline.cpp"
        "${SKIA_ROOT}/src/core/SkRasterPipelineBlitter.cpp"
        "${SKIA_ROOT}/src/core/SkReadBuffer.cpp"
        "${SKIA_ROOT}/src/core/SkReadPixelsRec.cpp"
        "${SKIA_ROOT}/src/core/SkRecord.cpp"
        "${SKIA_ROOT}/src/core/SkRecordCanvas.cpp"
        "${SKIA_ROOT}/src/core/SkRecordDraw.cpp"
        "${SKIA_ROOT}/src/core/SkRecordOpts.cpp"
        "${SKIA_ROOT}/src/core/SkRecordedDrawable.cpp"
        "${SKIA_ROOT}/src/core/SkRecords.cpp"
        "${SKIA_ROOT}/src/core/SkRect.cpp"
        "${SKIA_ROOT}/src/core/SkRegion.cpp"
        "${SKIA_ROOT}/src/core/SkRegion_path.cpp"
        "${SKIA_ROOT}/src/core/SkResourceCache.cpp"
        "${SKIA_ROOT}/src/core/SkRuntimeBlender.cpp"
        "${SKIA_ROOT}/src/core/SkRuntimeEffect.cpp"
        "${SKIA_ROOT}/src/core/SkSLTypeShared.cpp"
        "${SKIA_ROOT}/src/core/SkScalar.cpp"
        "${SKIA_ROOT}/src/core/SkScalerContext.cpp"
        "${SKIA_ROOT}/src/core/SkScan.cpp"
        "${SKIA_ROOT}/src/core/SkScan_AAAPath.cpp"
        "${SKIA_ROOT}/src/core/SkScan_AntiPath.cpp"
        "${SKIA_ROOT}/src/core/SkScan_Antihair.cpp"
        "${SKIA_ROOT}/src/core/SkScan_Hairline.cpp"
        "${SKIA_ROOT}/src/core/SkScan_Path.cpp"
        "${SKIA_ROOT}/src/core/SkSpecialImage.cpp"
        "${SKIA_ROOT}/src/core/SkSpriteBlitter_ARGB32.cpp"
        "${SKIA_ROOT}/src/core/SkStream.cpp"
        "${SKIA_ROOT}/src/core/SkStrike.cpp"
        "${SKIA_ROOT}/src/core/SkStrikeCache.cpp"
        "${SKIA_ROOT}/src/core/SkStrikeRef.cpp"
        "${SKIA_ROOT}/src/core/SkStrikeSpec.cpp"
        "${SKIA_ROOT}/src/core/SkString.cpp"
        "${SKIA_ROOT}/src/core/SkStringUtils.cpp"
        "${SKIA_ROOT}/src/core/SkStroke.cpp"
        "${SKIA_ROOT}/src/core/SkStrokeRec.cpp"
        "${SKIA_ROOT}/src/core/SkStrokerPriv.cpp"
        "${SKIA_ROOT}/src/core/SkSwizzle.cpp"
        "${SKIA_ROOT}/src/core/SkSwizzler_opts.cpp"
        "${SKIA_ROOT}/src/core/SkSwizzler_opts_lasx.cpp"
        "${SKIA_ROOT}/src/core/SkSwizzler_opts_ml3.cpp"
        "${SKIA_ROOT}/src/core/SkSwizzler_opts_ssse3.cpp"
        "${SKIA_ROOT}/src/core/SkSynchronizedResourceCache.cpp"
        "${SKIA_ROOT}/src/core/SkTaskGroup.cpp"
        "${SKIA_ROOT}/src/core/SkTextBlob.cpp"
        "${SKIA_ROOT}/src/core/SkTypeface.cpp"
        "${SKIA_ROOT}/src/core/SkTypefaceCache.cpp"
        "${SKIA_ROOT}/src/core/SkTypeface_remote.cpp"
        "${SKIA_ROOT}/src/core/SkUnPreMultiply.cpp"
        "${SKIA_ROOT}/src/core/SkVertState.cpp"
        "${SKIA_ROOT}/src/core/SkVertices.cpp"
        "${SKIA_ROOT}/src/core/SkWriteBuffer.cpp"
        "${SKIA_ROOT}/src/core/SkWritePixelsRec.cpp"
        "${SKIA_ROOT}/src/core/SkWriter32.cpp"
        "${SKIA_ROOT}/src/core/SkYUVAInfo.cpp"
        "${SKIA_ROOT}/src/core/SkYUVAPixmaps.cpp"
        "${SKIA_ROOT}/src/core/SkYUVMath.cpp"
        "${SKIA_ROOT}/src/core/SkYUVPlanesCache.cpp"
        "${SKIA_ROOT}/src/image/SkImage.cpp"
        "${SKIA_ROOT}/src/image/SkImage_Base.cpp"
        "${SKIA_ROOT}/src/image/SkImage_Lazy.cpp"
        "${SKIA_ROOT}/src/image/SkImage_LazyFactories.cpp"
        "${SKIA_ROOT}/src/image/SkImage_Picture.cpp"
        "${SKIA_ROOT}/src/image/SkImage_Raster.cpp"
        "${SKIA_ROOT}/src/image/SkImage_RasterFactories.cpp"
        "${SKIA_ROOT}/src/image/SkPictureImageGenerator.cpp"
        "${SKIA_ROOT}/src/image/SkRescaleAndReadPixels.cpp"
        "${SKIA_ROOT}/src/image/SkSurface.cpp"
        "${SKIA_ROOT}/src/image/SkSurface_Base.cpp"
        "${SKIA_ROOT}/src/image/SkSurface_Null.cpp"
        "${SKIA_ROOT}/src/image/SkSurface_Raster.cpp"
        "${SKIA_ROOT}/src/image/SkTiledImageUtils.cpp"
        "${SKIA_ROOT}/src/lazy/SkDiscardableMemoryPool.cpp"
        "${SKIA_ROOT}/src/shaders/SkBitmapProcShader.cpp"
        "${SKIA_ROOT}/src/shaders/SkBlendShader.cpp"
        "${SKIA_ROOT}/src/shaders/SkColorFilterShader.cpp"
        "${SKIA_ROOT}/src/shaders/SkColorShader.cpp"
        "${SKIA_ROOT}/src/shaders/SkCoordClampShader.cpp"
        "${SKIA_ROOT}/src/shaders/SkEmptyShader.cpp"
        "${SKIA_ROOT}/src/shaders/SkGainmapShader.cpp"
        "${SKIA_ROOT}/src/shaders/SkImageShader.cpp"
        "${SKIA_ROOT}/src/shaders/SkLocalMatrixShader.cpp"
        "${SKIA_ROOT}/src/shaders/SkPerlinNoiseShaderImpl.cpp"
        "${SKIA_ROOT}/src/shaders/SkPictureShader.cpp"
        "${SKIA_ROOT}/src/shaders/SkRuntimeShader.cpp"
        "${SKIA_ROOT}/src/shaders/SkShader.cpp"
        "${SKIA_ROOT}/src/shaders/SkShaderBase.cpp"
        "${SKIA_ROOT}/src/shaders/SkTransformShader.cpp"
        "${SKIA_ROOT}/src/shaders/SkTriColorShader.cpp"
        "${SKIA_ROOT}/src/shaders/SkWorkingColorSpaceShader.cpp"
        "${SKIA_ROOT}/src/text/GlyphRun.cpp"
        "${SKIA_ROOT}/src/text/SlugFromBuffer.cpp"
        "${SKIA_ROOT}/src/text/StrikeForGPU.cpp"
    )

    set(SKIA_EFFECTS_HEADERS
        "${SKIA_ROOT}/src/effects/SkDashImpl.h"
        "${SKIA_ROOT}/src/effects/SkEmbossMask.h"
        "${SKIA_ROOT}/src/effects/SkEmbossMaskFilter.h"
        "${SKIA_ROOT}/src/effects/SkShaderMaskFilterImpl.h"
        "${SKIA_ROOT}/src/effects/SkTrimPE.h"
        "${SKIA_ROOT}/src/shaders/gradients/SkConicalGradient.h"
        "${SKIA_ROOT}/src/shaders/gradients/SkGradientBaseShader.h"
        "${SKIA_ROOT}/src/shaders/gradients/SkLinearGradient.h"
        "${SKIA_ROOT}/src/shaders/gradients/SkRadialGradient.h"
        "${SKIA_ROOT}/src/shaders/gradients/SkSweepGradient.h"
    )

    set(SKIA_EFFECTS_SOURCES
        "${SKIA_ROOT}/src/effects/Sk1DPathEffect.cpp"
        "${SKIA_ROOT}/src/effects/Sk2DPathEffect.cpp"
        "${SKIA_ROOT}/src/effects/SkBlenders.cpp"
        "${SKIA_ROOT}/src/effects/SkColorMatrix.cpp"
        "${SKIA_ROOT}/src/effects/SkColorMatrixFilter.cpp"
        "${SKIA_ROOT}/src/effects/SkCornerPathEffect.cpp"
        "${SKIA_ROOT}/src/effects/SkDashPathEffect.cpp"
        "${SKIA_ROOT}/src/effects/SkDiscretePathEffect.cpp"
        "${SKIA_ROOT}/src/effects/SkEmbossMask.cpp"
        "${SKIA_ROOT}/src/effects/SkEmbossMaskFilter.cpp"
        "${SKIA_ROOT}/src/effects/SkHighContrastFilter.cpp"
        "${SKIA_ROOT}/src/effects/SkShaderMaskFilterImpl.cpp"
        "${SKIA_ROOT}/src/effects/SkTableMaskFilter.cpp"
        "${SKIA_ROOT}/src/effects/SkTrimPathEffect.cpp"
        "${SKIA_ROOT}/src/shaders/gradients/SkConicalGradient.cpp"
        "${SKIA_ROOT}/src/shaders/gradients/SkGradientBaseShader.cpp"
        "${SKIA_ROOT}/src/shaders/gradients/SkLinearGradient.cpp"
        "${SKIA_ROOT}/src/shaders/gradients/SkRadialGradient.cpp"
        "${SKIA_ROOT}/src/shaders/gradients/SkSweepGradient.cpp"
    )

    set(SKIA_COLORFILTER_HEADERS
        "${SKIA_ROOT}/src/effects/colorfilters/SkBlendModeColorFilter.h"
        "${SKIA_ROOT}/src/effects/colorfilters/SkColorFilterBase.h"
        "${SKIA_ROOT}/src/effects/colorfilters/SkColorSpaceXformColorFilter.h"
        "${SKIA_ROOT}/src/effects/colorfilters/SkComposeColorFilter.h"
        "${SKIA_ROOT}/src/effects/colorfilters/SkGaussianColorFilter.h"
        "${SKIA_ROOT}/src/effects/colorfilters/SkMatrixColorFilter.h"
        "${SKIA_ROOT}/src/effects/colorfilters/SkRuntimeColorFilter.h"
        "${SKIA_ROOT}/src/effects/colorfilters/SkTableColorFilter.h"
        "${SKIA_ROOT}/src/effects/colorfilters/SkWorkingFormatColorFilter.h"
    )

    set(SKIA_COLORFILTER_SOURCES
        "${SKIA_ROOT}/src/effects/colorfilters/SkBlendModeColorFilter.cpp"
        "${SKIA_ROOT}/src/effects/colorfilters/SkColorFilterBase.cpp"
        "${SKIA_ROOT}/src/effects/colorfilters/SkColorSpaceXformColorFilter.cpp"
        "${SKIA_ROOT}/src/effects/colorfilters/SkComposeColorFilter.cpp"
        "${SKIA_ROOT}/src/effects/colorfilters/SkGaussianColorFilter.cpp"
        "${SKIA_ROOT}/src/effects/colorfilters/SkMatrixColorFilter.cpp"
        "${SKIA_ROOT}/src/effects/colorfilters/SkRuntimeColorFilter.cpp"
        "${SKIA_ROOT}/src/effects/colorfilters/SkTableColorFilter.cpp"
        "${SKIA_ROOT}/src/effects/colorfilters/SkWorkingFormatColorFilter.cpp"
    )

    set(SKIA_IMAGEFILTER_HEADERS
        "${SKIA_ROOT}/src/effects/imagefilters/SkMatrixConvolutionImageFilter.h"
    )

    set(SKIA_IMAGEFILTER_SOURCES
        "${SKIA_ROOT}/src/effects/imagefilters/SkBlendImageFilter.cpp"
        "${SKIA_ROOT}/src/effects/imagefilters/SkBlurImageFilter.cpp"
        "${SKIA_ROOT}/src/effects/imagefilters/SkColorFilterImageFilter.cpp"
        "${SKIA_ROOT}/src/effects/imagefilters/SkComposeImageFilter.cpp"
        "${SKIA_ROOT}/src/effects/imagefilters/SkCropImageFilter.cpp"
        "${SKIA_ROOT}/src/effects/imagefilters/SkDisplacementMapImageFilter.cpp"
        "${SKIA_ROOT}/src/effects/imagefilters/SkDropShadowImageFilter.cpp"
        "${SKIA_ROOT}/src/effects/imagefilters/SkImageImageFilter.cpp"
        "${SKIA_ROOT}/src/effects/imagefilters/SkLightingImageFilter.cpp"
        "${SKIA_ROOT}/src/effects/imagefilters/SkMagnifierImageFilter.cpp"
        "${SKIA_ROOT}/src/effects/imagefilters/SkMatrixConvolutionImageFilter.cpp"
        "${SKIA_ROOT}/src/effects/imagefilters/SkMatrixTransformImageFilter.cpp"
        "${SKIA_ROOT}/src/effects/imagefilters/SkMergeImageFilter.cpp"
        "${SKIA_ROOT}/src/effects/imagefilters/SkMorphologyImageFilter.cpp"
        "${SKIA_ROOT}/src/effects/imagefilters/SkPictureImageFilter.cpp"
        "${SKIA_ROOT}/src/effects/imagefilters/SkRuntimeImageFilter.cpp"
        "${SKIA_ROOT}/src/effects/imagefilters/SkShaderImageFilter.cpp"
    )

    set(SKIA_CODEC_HEADERS
        "${SKIA_ROOT}/include/codec/SkCodec.h"
        "${SKIA_ROOT}/include/codec/SkCodecAnimation.h"
        "${SKIA_ROOT}/include/codec/SkEncodedImageFormat.h"
        "${SKIA_ROOT}/include/codec/SkPixmapUtils.h"
        "${SKIA_ROOT}/src/codec/SkCodecImageGenerator.h"
        "${SKIA_ROOT}/src/codec/SkCodecPriv.h"
        "${SKIA_ROOT}/src/codec/SkColorPalette.h"
        "${SKIA_ROOT}/src/codec/SkFrameHolder.h"
        "${SKIA_ROOT}/src/codec/SkHdrAgtmPriv.h"
        "${SKIA_ROOT}/src/codec/SkMaskSwizzler.h"
        "${SKIA_ROOT}/src/codec/SkParseEncodedOrigin.h"
        "${SKIA_ROOT}/src/codec/SkPixmapUtilsPriv.h"
        "${SKIA_ROOT}/src/codec/SkSampler.h"
        "${SKIA_ROOT}/src/codec/SkScalingCodec.h"
        "${SKIA_ROOT}/src/codec/SkSwizzler.h"
        "${SKIA_ROOT}/src/codec/SkTiffUtility.h"
    )

    set(SKIA_CODEC_SOURCES
        "${SKIA_ROOT}/src/codec/SkCodec.cpp"
        "${SKIA_ROOT}/src/codec/SkCodecColorProfile.cpp"
        "${SKIA_ROOT}/src/codec/SkCodecImageGenerator.cpp"
        "${SKIA_ROOT}/src/codec/SkColorPalette.cpp"
        "${SKIA_ROOT}/src/codec/SkEncodedInfo.cpp"
        "${SKIA_ROOT}/src/codec/SkExif.cpp"
        "${SKIA_ROOT}/src/codec/SkGainmapInfo.cpp"
        "${SKIA_ROOT}/src/codec/SkHdrAgtm.cpp"
        "${SKIA_ROOT}/src/codec/SkHdrAgtmParse.cpp"
        "${SKIA_ROOT}/src/codec/SkHdrMetadata.cpp"
        "${SKIA_ROOT}/src/codec/SkImageGenerator_FromEncoded.cpp"
        "${SKIA_ROOT}/src/codec/SkMaskSwizzler.cpp"
        "${SKIA_ROOT}/src/codec/SkParseEncodedOrigin.cpp"
        "${SKIA_ROOT}/src/codec/SkPixmapUtils.cpp"
        "${SKIA_ROOT}/src/codec/SkSampler.cpp"
        "${SKIA_ROOT}/src/codec/SkSwizzler.cpp"
        "${SKIA_ROOT}/src/codec/SkTiffUtility.cpp"
        "${SKIA_ROOT}/src/codec/SkBmpCodec.cpp"
        "${SKIA_ROOT}/src/codec/SkBmpBaseCodec.cpp"
        "${SKIA_ROOT}/src/codec/SkBmpMaskCodec.cpp"
        "${SKIA_ROOT}/src/codec/SkBmpRLECodec.cpp"
        "${SKIA_ROOT}/src/codec/SkBmpStandardCodec.cpp"
        "${SKIA_ROOT}/src/codec/SkWbmpCodec.cpp"
    )

    set(SKIA_PNG_CODEC_HEADERS
        "${SKIA_ROOT}/src/codec/SkBmpBaseCodec.h"
        "${SKIA_ROOT}/src/codec/SkBmpCodec.h"
        "${SKIA_ROOT}/src/codec/SkBmpMaskCodec.h"
        "${SKIA_ROOT}/src/codec/SkBmpRLECodec.h"
        "${SKIA_ROOT}/src/codec/SkBmpStandardCodec.h"
        "${SKIA_ROOT}/src/codec/SkWbmpCodec.h"
    )

    set(SKIA_ENCODE_HEADERS
        "${SKIA_ROOT}/src/encode/SkICCPriv.h"
        "${SKIA_ROOT}/src/encode/SkImageEncoderFns.h"
        "${SKIA_ROOT}/src/encode/SkImageEncoderPriv.h"
    )

    set(SKIA_ENCODE_SOURCES
        "${SKIA_ROOT}/src/encode/SkEncoder.cpp"
        "${SKIA_ROOT}/src/encode/SkICC.cpp"
    )

    set(SKIA_GPU_HEADERS
        "${SKIA_ROOT}/include/gpu/GpuTypes.h"
        "${SKIA_ROOT}/include/gpu/MutableTextureState.h"
        "${SKIA_ROOT}/include/gpu/ShaderErrorHandler.h"
        "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLAssembleHelpers.h"
        "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLAssembleInterface.h"
        "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLBackendSurface.h"
        "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLConfig.h"
        "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLDirectContext.h"
        "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLExtensions.h"
        "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLFunctions.h"
        "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLInterface.h"
        "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLTypes.h"
        "${SKIA_ROOT}/include/private/base/SingleOwner.h"
        "${SKIA_ROOT}/include/private/chromium/Slug.h"
        "${SKIA_ROOT}/include/private/gpu/ganesh/GrContext_Base.h"
        "${SKIA_ROOT}/include/private/gpu/ganesh/GrImageContext.h"
        "${SKIA_ROOT}/include/private/gpu/ganesh/GrTextureGenerator.h"
        "${SKIA_ROOT}/include/private/gpu/ganesh/GrTypesPriv.h"
        "${SKIA_ROOT}/src/gpu/AsyncReadTypes.h"
        "${SKIA_ROOT}/src/gpu/Blend.h"
        "${SKIA_ROOT}/src/gpu/BlendFormula.h"
        "${SKIA_ROOT}/src/gpu/BlurUtils.h"
        "${SKIA_ROOT}/src/gpu/BufferWriter.h"
        "${SKIA_ROOT}/src/gpu/DataUtils.h"
        "${SKIA_ROOT}/src/gpu/DitherUtils.h"
        "${SKIA_ROOT}/src/gpu/GpuRefCnt.h"
        "${SKIA_ROOT}/src/gpu/GpuTypesPriv.h"
        "${SKIA_ROOT}/src/gpu/KeyBuilder.h"
        "${SKIA_ROOT}/src/gpu/MaskFormat.h"
        "${SKIA_ROOT}/src/gpu/MutableTextureStatePriv.h"
        "${SKIA_ROOT}/src/gpu/Rectanizer.h"
        "${SKIA_ROOT}/src/gpu/RectanizerPow2.h"
        "${SKIA_ROOT}/src/gpu/RectanizerSkyline.h"
        "${SKIA_ROOT}/src/gpu/RefCntedCallback.h"
        "${SKIA_ROOT}/src/gpu/ResourceKey.h"
        "${SKIA_ROOT}/src/gpu/SkBackingFit.h"
        "${SKIA_ROOT}/src/gpu/SkRenderEngineAbortf.h"
        "${SKIA_ROOT}/src/gpu/SkSLToBackend.h"
        "${SKIA_ROOT}/src/gpu/Swizzle.h"
        "${SKIA_ROOT}/src/gpu/SwizzlePriv.h"
        "${SKIA_ROOT}/src/gpu/TiledTextureUtils.h"
        "${SKIA_ROOT}/src/gpu/Token.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ClipStack.h"
        "${SKIA_ROOT}/src/gpu/ganesh/Device.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrAppliedClip.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrAtlasTypes.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrAttachment.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrAuditTrail.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBackendSemaphorePriv.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBackendSurfacePriv.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBackendTextureImageGenerator.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBackendUtils.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBaseContextPriv.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBlurUtils.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBuffer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBufferAllocPool.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBufferTransferRenderTask.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBufferUpdateRenderTask.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrCanvas.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrCaps.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrClientMappedBufferManager.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrClip.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrColor.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrColorInfo.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrColorSpaceXform.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrContextThreadSafeProxyPriv.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrCopyRenderTask.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrCpuBuffer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDDLTask.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDataUtils.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDefaultGeoProcFactory.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDeferredDisplayListPriv.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDeferredProxyUploader.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDeferredUpload.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDirectContextPriv.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDistanceFieldGenFromVector.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDrawIndirectCommand.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDrawOpAtlas.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDrawOpTest.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDrawingManager.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDstProxyView.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDynamicAtlas.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrEagerVertexAllocator.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrFPArgs.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrFixedClip.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrFragmentProcessor.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrFragmentProcessors.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrGeometryProcessor.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrGpu.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrGpuBuffer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrGpuResource.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrGpuResourceCacheAccess.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrGpuResourcePriv.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrHashMapWithCache.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrImageContextPriv.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrImageInfo.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrManagedResource.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrMemoryPool.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrMeshBuffers.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrMeshDrawTarget.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrNativeRect.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrNonAtomicRef.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrOnFlushResourceProvider.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrOpFlushState.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrOpsRenderPass.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrOpsTypes.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrPaint.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrPersistentCacheUtils.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrPipeline.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrPixmap.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrProcessor.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrProcessorAnalysis.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrProcessorSet.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrProcessorUnitTest.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrProgramDesc.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrProgramInfo.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrProxyProvider.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrRecordingContextPriv.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrRenderTarget.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrRenderTargetProxy.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrRenderTask.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrRenderTaskCluster.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrResourceAllocator.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrResourceCache.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrResourceHandle.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrResourceProvider.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrResourceProviderPriv.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrRingBuffer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrSPIRVUniformHandler.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrSPIRVVaryingHandler.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrSWMaskHelper.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrSamplerState.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrScissorState.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrSemaphore.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrShaderCaps.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrShaderVar.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrSimpleMesh.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrStagingBufferManager.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrStencilSettings.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrStyle.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrSurface.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrSurfaceProxy.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrSurfaceProxyPriv.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrSurfaceProxyView.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTTopoSort.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTestUtils.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTexture.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTextureProxy.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTextureProxyCacheAccess.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTextureProxyPriv.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTextureRenderTargetProxy.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTextureResolveManager.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTextureResolveRenderTask.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrThreadSafeCache.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrThreadSafePipelineBuilder.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTracing.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTransferFromRenderTask.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrUniformDataManager.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrUserStencilSettings.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrUtil.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrVertexChunkArray.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrWaitRenderTask.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrWindowRectangles.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrWindowRectsState.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrWritePixelsRenderTask.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrXferProcessor.h"
        "${SKIA_ROOT}/src/gpu/ganesh/GrYUVATextureProxies.h"
        "${SKIA_ROOT}/src/gpu/ganesh/PathRenderer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/PathRendererChain.h"
        "${SKIA_ROOT}/src/gpu/ganesh/SkGaneshRecorder.h"
        "${SKIA_ROOT}/src/gpu/ganesh/SkGr.h"
        "${SKIA_ROOT}/src/gpu/ganesh/StencilClip.h"
        "${SKIA_ROOT}/src/gpu/ganesh/StencilMaskHelper.h"
        "${SKIA_ROOT}/src/gpu/ganesh/SurfaceContext.h"
        "${SKIA_ROOT}/src/gpu/ganesh/SurfaceDrawContext.h"
        "${SKIA_ROOT}/src/gpu/ganesh/SurfaceFillContext.h"
        "${SKIA_ROOT}/src/gpu/ganesh/TestFormatColorTypeCombination.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrAtlasedShaderHelpers.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrBezierEffect.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrBicubicEffect.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrBitmapTextGeoProc.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrBlendFragmentProcessor.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrColorTableEffect.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrConvexPolyEffect.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrCoverageSetOpXP.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrCustomXfermode.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrDisableColorXP.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrDistanceFieldGeoProc.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrMatrixEffect.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrModulateAtlasCoverageEffect.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrOvalEffect.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrPerlinNoise2Effect.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrPorterDuffXferProcessor.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrRRectEffect.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrShadowGeoProc.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrSkSLFP.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrTextureEffect.h"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrYUVtoRGBEffect.h"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrAAConvexTessellator.h"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrAATriangulator.h"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrInnerFanTriangulator.h"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrPathUtils.h"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrQuad.h"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrQuadBuffer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrQuadUtils.h"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrRect.h"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrShape.h"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrStyledShape.h"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrTriangulator.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLAttachment.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLBackendSurfacePriv.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLBuffer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLCaps.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLContext.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLCoreFunctions.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLDefines.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLFinishCallbacks.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLGLSL.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLGpu.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLOpsRenderPass.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLProgram.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLProgramDataManager.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLRenderTarget.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLSemaphore.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLTexture.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLTextureRenderTarget.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLTypesPriv.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLUniformHandler.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLUtil.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLVaryingHandler.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLVertexArray.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/builders/GrGLProgramBuilder.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/builders/GrGLShaderStringBuilder.h"
        "${SKIA_ROOT}/src/gpu/ganesh/glsl/GrGLSLBlend.h"
        "${SKIA_ROOT}/src/gpu/ganesh/glsl/GrGLSLColorSpaceXformHelper.h"
        "${SKIA_ROOT}/src/gpu/ganesh/glsl/GrGLSLFragmentShaderBuilder.h"
        "${SKIA_ROOT}/src/gpu/ganesh/glsl/GrGLSLProgramBuilder.h"
        "${SKIA_ROOT}/src/gpu/ganesh/glsl/GrGLSLProgramDataManager.h"
        "${SKIA_ROOT}/src/gpu/ganesh/glsl/GrGLSLShaderBuilder.h"
        "${SKIA_ROOT}/src/gpu/ganesh/glsl/GrGLSLUniformHandler.h"
        "${SKIA_ROOT}/src/gpu/ganesh/glsl/GrGLSLVarying.h"
        "${SKIA_ROOT}/src/gpu/ganesh/glsl/GrGLSLVertexGeoBuilder.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gradients/GrGradientBitmapCache.h"
        "${SKIA_ROOT}/src/gpu/ganesh/gradients/GrGradientShader.h"
        "${SKIA_ROOT}/src/gpu/ganesh/image/GrImageUtils.h"
        "${SKIA_ROOT}/src/gpu/ganesh/image/GrMippedBitmap.h"
        "${SKIA_ROOT}/src/gpu/ganesh/image/SkImage_Ganesh.h"
        "${SKIA_ROOT}/src/gpu/ganesh/image/SkImage_GaneshBase.h"
        "${SKIA_ROOT}/src/gpu/ganesh/image/SkImage_GaneshYUVA.h"
        "${SKIA_ROOT}/src/gpu/ganesh/image/SkImage_LazyTexture.h"
        "${SKIA_ROOT}/src/gpu/ganesh/image/SkImage_RasterPinnable.h"
        "${SKIA_ROOT}/src/gpu/ganesh/image/SkSpecialImage_Ganesh.h"
        "${SKIA_ROOT}/src/gpu/ganesh/mock/GrMockAttachment.h"
        "${SKIA_ROOT}/src/gpu/ganesh/mock/GrMockBuffer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/mock/GrMockCaps.h"
        "${SKIA_ROOT}/src/gpu/ganesh/mock/GrMockGpu.h"
        "${SKIA_ROOT}/src/gpu/ganesh/mock/GrMockOpTarget.h"
        "${SKIA_ROOT}/src/gpu/ganesh/mock/GrMockOpsRenderPass.h"
        "${SKIA_ROOT}/src/gpu/ganesh/mock/GrMockRenderTask.h"
        "${SKIA_ROOT}/src/gpu/ganesh/mock/GrMockSurfaceProxy.h"
        "${SKIA_ROOT}/src/gpu/ganesh/mock/GrMockTexture.h"
        "${SKIA_ROOT}/src/gpu/ganesh/mock/GrMockTypesPriv.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/AAConvexPathRenderer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/AAHairLinePathRenderer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/AALinearizingConvexPathRenderer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/AtlasInstancedHelper.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/AtlasPathRenderer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/AtlasRenderTask.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/AtlasTextOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/ClearOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/DashLinePathRenderer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/DashOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/DefaultPathRenderer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/DrawAtlasOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/DrawAtlasPathOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/DrawMeshOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/DrawableOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/FillPathFlags.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/FillRRectOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/FillRectOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/GrDrawOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/GrMeshDrawOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/GrOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/GrOvalOpFactory.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/GrPathStencilSettings.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/GrSimpleMeshDrawOpHelper.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/GrSimpleMeshDrawOpHelperWithStencil.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/LatticeOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/OpsTask.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/PathInnerTriangulateOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/PathStencilCoverOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/PathTessellateOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/QuadPerEdgeAA.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/RegionOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/ShadowRRectOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/SmallPathAtlasMgr.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/SmallPathRenderer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/SmallPathShapeData.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/SoftwarePathRenderer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/StrokeRectOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/StrokeTessellateOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/TessellationPathRenderer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/TextureOp.h"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/TriangulatingPathRenderer.h"
        "${SKIA_ROOT}/src/gpu/ganesh/tessellate/GrPathTessellationShader.h"
        "${SKIA_ROOT}/src/gpu/ganesh/tessellate/GrStrokeTessellationShader.h"
        "${SKIA_ROOT}/src/gpu/ganesh/tessellate/GrTessellationShader.h"
        "${SKIA_ROOT}/src/gpu/ganesh/tessellate/PathTessellator.h"
        "${SKIA_ROOT}/src/gpu/ganesh/tessellate/StrokeTessellator.h"
        "${SKIA_ROOT}/src/gpu/ganesh/tessellate/VertexChunkPatchAllocator.h"
        "${SKIA_ROOT}/src/gpu/ganesh/text/GlyphData.h"
        "${SKIA_ROOT}/src/gpu/ganesh/text/GrAtlasManager.h"
        "${SKIA_ROOT}/src/gpu/ganesh/text/TextStrike.h"
        "${SKIA_ROOT}/src/gpu/tessellate/AffineMatrix.h"
        "${SKIA_ROOT}/src/gpu/tessellate/CullTest.h"
        "${SKIA_ROOT}/src/gpu/tessellate/FixedCountBufferUtils.h"
        "${SKIA_ROOT}/src/gpu/tessellate/LinearTolerances.h"
        "${SKIA_ROOT}/src/gpu/tessellate/MiddleOutPolygonTriangulator.h"
        "${SKIA_ROOT}/src/gpu/tessellate/MidpointContourParser.h"
        "${SKIA_ROOT}/src/gpu/tessellate/PatchWriter.h"
        "${SKIA_ROOT}/src/gpu/tessellate/StrokeIterator.h"
        "${SKIA_ROOT}/src/gpu/tessellate/Tessellation.h"
        "${SKIA_ROOT}/src/gpu/tessellate/WangsFormula.h"
        "${SKIA_ROOT}/src/text/gpu/DistanceFieldAdjustTable.h"
        "${SKIA_ROOT}/src/text/gpu/GlyphUtils.h"
        "${SKIA_ROOT}/src/text/gpu/GlyphVector.h"
        "${SKIA_ROOT}/src/text/gpu/SDFMaskFilter.h"
        "${SKIA_ROOT}/src/text/gpu/SlugImpl.h"
        "${SKIA_ROOT}/src/text/gpu/StrikeCache.h"
        "${SKIA_ROOT}/src/text/gpu/SubRunAllocator.h"
        "${SKIA_ROOT}/src/text/gpu/SubRunContainer.h"
        "${SKIA_ROOT}/src/text/gpu/SubRunControl.h"
        "${SKIA_ROOT}/src/text/gpu/TextBlob.h"
        "${SKIA_ROOT}/src/text/gpu/TextBlobRedrawCoordinator.h"
        "${SKIA_ROOT}/src/text/gpu/VertexFiller.h"
    )

    set(SKIA_GPU_SOURCES
        "${SKIA_ROOT}/src/gpu/Blend.cpp"
        "${SKIA_ROOT}/src/gpu/BlendFormula.cpp"
        "${SKIA_ROOT}/src/gpu/BlurUtils.cpp"
        "${SKIA_ROOT}/src/gpu/DataUtils.cpp"
        "${SKIA_ROOT}/src/gpu/DitherUtils.cpp"
        "${SKIA_ROOT}/src/gpu/MutableTextureState.cpp"
        "${SKIA_ROOT}/src/gpu/RectanizerPow2.cpp"
        "${SKIA_ROOT}/src/gpu/RectanizerSkyline.cpp"
        "${SKIA_ROOT}/src/gpu/ResourceKey.cpp"
        "${SKIA_ROOT}/src/gpu/ShaderErrorHandler.cpp"
        "${SKIA_ROOT}/src/gpu/SkBackingFit.cpp"
        "${SKIA_ROOT}/src/gpu/SkSLToBackend.cpp"
        "${SKIA_ROOT}/src/gpu/Swizzle.cpp"
        "${SKIA_ROOT}/src/gpu/TiledTextureUtils.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ClipStack.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/Device.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/Device_drawTexture.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrAtlasTypes.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrAttachment.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrAuditTrail.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBackendSemaphore.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBackendSurface.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBackendTextureImageGenerator.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBackendUtils.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBlurUtils.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBufferAllocPool.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBufferTransferRenderTask.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrBufferUpdateRenderTask.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrCanvas.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrCaps.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrClientMappedBufferManager.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrColorInfo.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrColorSpaceXform.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrContextThreadSafeProxy.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrContext_Base.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrCopyRenderTask.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDDLContext.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDDLTask.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDataUtils.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDefaultGeoProcFactory.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDeferredDisplayList.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDeferredDisplayListRecorder.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDirectContext.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDirectContextPriv.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDistanceFieldGenFromVector.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDrawOpAtlas.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDrawOpTest.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDrawingManager.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDriverBugWorkarounds.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrDynamicAtlas.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrEagerVertexAllocator.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrFixedClip.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrFragmentProcessor.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrFragmentProcessors.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrGeometryProcessor.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrGpu.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrGpuBuffer.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrGpuResource.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrImageContext.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrImageInfo.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrManagedResource.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrMemoryPool.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrMeshBuffers.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrMeshDrawTarget.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrOnFlushResourceProvider.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrOpFlushState.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrOpsRenderPass.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrPaint.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrPersistentCacheUtils.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrPipeline.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrProcessor.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrProcessorAnalysis.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrProcessorSet.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrProcessorUnitTest.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrProgramDesc.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrProgramInfo.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrPromiseImageTexture.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrProxyProvider.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrRecordingContext.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrRecordingContextPriv.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrRenderTarget.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrRenderTargetProxy.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrRenderTask.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrRenderTaskCluster.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrResourceAllocator.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrResourceCache.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrResourceProvider.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrRingBuffer.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrSPIRVUniformHandler.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrSPIRVVaryingHandler.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrSWMaskHelper.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrShaderCaps.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrShaderVar.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrStagingBufferManager.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrStencilSettings.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrStyle.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrSurface.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrSurfaceCharacterization.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrSurfaceProxy.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrSurfaceProxyView.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTestUtils.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTexture.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTextureProxy.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTextureProxyCacheAccess.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTextureRenderTargetProxy.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTextureResolveRenderTask.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrThreadSafeCache.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrThreadSafePipelineBuilder.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrTransferFromRenderTask.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrUniformDataManager.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrUtil.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrVertexChunkArray.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrWaitRenderTask.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrWritePixelsRenderTask.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrXferProcessor.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrYUVABackendTextures.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/GrYUVATextureProxies.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/PathRenderer.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/PathRendererChain.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/SkGr.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/StencilMaskHelper.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/SurfaceContext.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/SurfaceDrawContext.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/SurfaceFillContext.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrBezierEffect.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrBicubicEffect.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrBitmapTextGeoProc.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrBlendFragmentProcessor.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrColorTableEffect.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrConvexPolyEffect.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrCoverageSetOpXP.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrCustomXfermode.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrDisableColorXP.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrDistanceFieldGeoProc.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrMatrixEffect.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrModulateAtlasCoverageEffect.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrOvalEffect.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrPerlinNoise2Effect.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrPorterDuffXferProcessor.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrRRectEffect.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrShadowGeoProc.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrSkSLFP.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrTextureEffect.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/effects/GrYUVtoRGBEffect.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrAAConvexTessellator.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrAATriangulator.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrPathUtils.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrQuad.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrQuadUtils.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrShape.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrStyledShape.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/geometry/GrTriangulator.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLAssembleGLESInterfaceAutogen.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLAssembleGLInterfaceAutogen.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLAssembleHelpers.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLAssembleInterface.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLAssembleWebGLInterfaceAutogen.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLAttachment.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLBackendSurface.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLBuffer.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLCaps.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLContext.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLDirectContext.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLExtensions.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLFinishCallbacks.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLGLSL.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLGpu.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLGpuProgramCache.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLInterfaceAutogen.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLOpsRenderPass.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLProgram.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLProgramDataManager.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLRenderTarget.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLSemaphore.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLTexture.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLTextureRenderTarget.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLTypesPriv.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLUniformHandler.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLUtil.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/GrGLVertexArray.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/builders/GrGLProgramBuilder.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gl/builders/GrGLShaderStringBuilder.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/glsl/GrGLSLBlend.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/glsl/GrGLSLFragmentShaderBuilder.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/glsl/GrGLSLProgramBuilder.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/glsl/GrGLSLProgramDataManager.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/glsl/GrGLSLShaderBuilder.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/glsl/GrGLSLUniformHandler.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/glsl/GrGLSLVarying.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/glsl/GrGLSLVertexGeoBuilder.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gradients/GrGradientBitmapCache.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/gradients/GrGradientShader.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/image/GrImageUtils.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/image/GrMippedBitmap.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/image/GrTextureGenerator.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/image/SkImage_Ganesh.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/image/SkImage_GaneshBase.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/image/SkImage_GaneshFactories.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/image/SkImage_GaneshYUVA.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/image/SkImage_LazyTexture.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/image/SkImage_RasterPinnable.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/image/SkSpecialImage_Ganesh.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/mock/GrMockBackendSurface.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/mock/GrMockCaps.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/mock/GrMockGpu.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/mock/GrMockTypes.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/AAConvexPathRenderer.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/AAHairLinePathRenderer.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/AALinearizingConvexPathRenderer.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/AtlasInstancedHelper.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/AtlasPathRenderer.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/AtlasRenderTask.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/AtlasTextOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/ClearOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/DashLinePathRenderer.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/DashOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/DefaultPathRenderer.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/DrawAtlasOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/DrawAtlasPathOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/DrawMeshOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/DrawableOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/FillRRectOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/FillRectOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/GrMeshDrawOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/GrOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/GrOvalOpFactory.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/GrSimpleMeshDrawOpHelper.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/GrSimpleMeshDrawOpHelperWithStencil.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/LatticeOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/OpsTask.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/PathInnerTriangulateOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/PathStencilCoverOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/PathTessellateOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/QuadPerEdgeAA.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/RegionOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/ShadowRRectOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/SmallPathAtlasMgr.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/SmallPathRenderer.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/SmallPathShapeData.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/SoftwarePathRenderer.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/StrokeRectOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/StrokeTessellateOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/TessellationPathRenderer.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/TextureOp.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/ops/TriangulatingPathRenderer.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/surface/SkSurface_Ganesh.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/tessellate/GrPathTessellationShader.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/tessellate/GrStrokeTessellationShader.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/tessellate/GrTessellationShader.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/tessellate/PathTessellator.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/tessellate/StrokeTessellator.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/text/GlyphData.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/text/GrAtlasManager.cpp"
        "${SKIA_ROOT}/src/gpu/ganesh/text/TextStrike.cpp"
        "${SKIA_ROOT}/src/gpu/tessellate/FixedCountBufferUtils.cpp"
        "${SKIA_ROOT}/src/gpu/tessellate/Tessellation.cpp"
        "${SKIA_ROOT}/src/text/gpu/DistanceFieldAdjustTable.cpp"
        "${SKIA_ROOT}/src/text/gpu/GlyphVector.cpp"
        "${SKIA_ROOT}/src/text/gpu/SDFMaskFilter.cpp"
        "${SKIA_ROOT}/src/text/gpu/SkChromeRemoteGlyphCache.cpp"
        "${SKIA_ROOT}/src/text/gpu/Slug.cpp"
        "${SKIA_ROOT}/src/text/gpu/SlugImpl.cpp"
        "${SKIA_ROOT}/src/text/gpu/StrikeCache.cpp"
        "${SKIA_ROOT}/src/text/gpu/SubRunAllocator.cpp"
        "${SKIA_ROOT}/src/text/gpu/SubRunContainer.cpp"
        "${SKIA_ROOT}/src/text/gpu/SubRunControl.cpp"
        "${SKIA_ROOT}/src/text/gpu/TextBlob.cpp"
        "${SKIA_ROOT}/src/text/gpu/TextBlobRedrawCoordinator.cpp"
        "${SKIA_ROOT}/src/text/gpu/VertexFiller.cpp"
    )

    set(SKIA_SKSL_HEADERS
        "${SKIA_ROOT}/include/sksl/SkSLDebugTrace.h"
        "${SKIA_ROOT}/include/sksl/SkSLVersion.h"
        "${SKIA_ROOT}/src/sksl/SkSLAnalysis.h"
        "${SKIA_ROOT}/src/sksl/SkSLBuiltinTypes.h"
        "${SKIA_ROOT}/src/sksl/SkSLCompiler.h"
        "${SKIA_ROOT}/src/sksl/SkSLConstantFolder.h"
        "${SKIA_ROOT}/src/sksl/SkSLContext.h"
        "${SKIA_ROOT}/src/sksl/SkSLDefines.h"
        "${SKIA_ROOT}/src/sksl/SkSLErrorReporter.h"
        "${SKIA_ROOT}/src/sksl/SkSLFileOutputStream.h"
        "${SKIA_ROOT}/src/sksl/SkSLGLSL.h"
        "${SKIA_ROOT}/src/sksl/SkSLGraphiteModules.h"
        "${SKIA_ROOT}/src/sksl/SkSLInliner.h"
        "${SKIA_ROOT}/src/sksl/SkSLIntrinsicList.h"
        "${SKIA_ROOT}/src/sksl/SkSLLexer.h"
        "${SKIA_ROOT}/src/sksl/SkSLMangler.h"
        "${SKIA_ROOT}/src/sksl/SkSLMemoryLayout.h"
        "${SKIA_ROOT}/src/sksl/SkSLMemoryPool.h"
        "${SKIA_ROOT}/src/sksl/SkSLModule.h"
        "${SKIA_ROOT}/src/sksl/SkSLModuleLoader.h"
        "${SKIA_ROOT}/src/sksl/SkSLOperator.h"
        "${SKIA_ROOT}/src/sksl/SkSLOutputStream.h"
        "${SKIA_ROOT}/src/sksl/SkSLParser.h"
        "${SKIA_ROOT}/src/sksl/SkSLPool.h"
        "${SKIA_ROOT}/src/sksl/SkSLPosition.h"
        "${SKIA_ROOT}/src/sksl/SkSLProgramKind.h"
        "${SKIA_ROOT}/src/sksl/SkSLProgramSettings.h"
        "${SKIA_ROOT}/src/sksl/SkSLString.h"
        "${SKIA_ROOT}/src/sksl/SkSLStringStream.h"
        "${SKIA_ROOT}/src/sksl/SkSLUtil.h"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLNoOpErrorReporter.h"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLProgramUsage.h"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLProgramVisitor.h"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLSpecialization.h"
        "${SKIA_ROOT}/src/sksl/codegen/SkSLCodeGenTypes.h"
        "${SKIA_ROOT}/src/sksl/codegen/SkSLCodeGenerator.h"
        "${SKIA_ROOT}/src/sksl/codegen/SkSLGLSLCodeGenerator.h"
        "${SKIA_ROOT}/src/sksl/codegen/SkSLMetalCodeGenerator.h"
        "${SKIA_ROOT}/src/sksl/codegen/SkSLNativeShader.h"
        "${SKIA_ROOT}/src/sksl/codegen/SkSLPipelineStageCodeGenerator.h"
        "${SKIA_ROOT}/src/sksl/codegen/SkSLRasterPipelineBuilder.h"
        "${SKIA_ROOT}/src/sksl/codegen/SkSLRasterPipelineCodeGenerator.h"
        "${SKIA_ROOT}/src/sksl/codegen/SkSLSPIRVCodeGenerator.h"
        "${SKIA_ROOT}/src/sksl/codegen/SkSLWGSLCodeGenerator.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLBinaryExpression.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLBlock.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLBreakStatement.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLChildCall.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructor.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorArray.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorArrayCast.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorCompound.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorCompoundCast.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorDiagonalMatrix.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorMatrixResize.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorScalarCast.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorSplat.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorStruct.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLContinueStatement.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLDiscardStatement.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLDoStatement.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLEmptyExpression.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLExpression.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLExpressionStatement.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLExtension.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLFieldAccess.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLFieldSymbol.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLForStatement.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLFunctionCall.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLFunctionDeclaration.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLFunctionDefinition.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLFunctionPrototype.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLFunctionReference.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLIRHelpers.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLIRNode.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLIfStatement.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLIndexExpression.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLInterfaceBlock.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLLayout.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLLiteral.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLMethodReference.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLModifierFlags.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLModifiers.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLModifiersDeclaration.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLNop.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLPoison.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLPostfixExpression.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLPrefixExpression.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLProgram.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLProgramElement.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLReturnStatement.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLSetting.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLStatement.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLStructDefinition.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLSwitchCase.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLSwitchStatement.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLSwizzle.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLSymbol.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLSymbolTable.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLTernaryExpression.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLType.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLTypeReference.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLVarDeclarations.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLVariable.h"
        "${SKIA_ROOT}/src/sksl/ir/SkSLVariableReference.h"
        "${SKIA_ROOT}/src/sksl/spirv.h"
        "${SKIA_ROOT}/src/sksl/tracing/SkSLDebugTracePlayer.h"
        "${SKIA_ROOT}/src/sksl/tracing/SkSLDebugTracePriv.h"
        "${SKIA_ROOT}/src/sksl/tracing/SkSLTraceHook.h"
        "${SKIA_ROOT}/src/sksl/transform/SkSLProgramWriter.h"
        "${SKIA_ROOT}/src/sksl/transform/SkSLTransform.h"
    )

    set(SKIA_SKSL_SOURCES
        "${SKIA_ROOT}/src/sksl/SkSLAnalysis.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLBuiltinTypes.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLCompiler.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLConstantFolder.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLContext.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLErrorReporter.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLInliner.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLIntrinsicList.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLLexer.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLMangler.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLModule.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLModuleDataDefault.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLModuleLoader.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLOperator.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLOutputStream.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLParser.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLPool.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLPosition.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLSampleUsage.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLString.cpp"
        "${SKIA_ROOT}/src/sksl/SkSLUtil.cpp"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLCanExitWithoutReturningValue.cpp"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLCheckProgramStructure.cpp"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLCheckSymbolTableCorrectness.cpp"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLFinalizationChecks.cpp"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLGetLoopControlFlowInfo.cpp"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLGetLoopUnrollInfo.cpp"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLGetReturnComplexity.cpp"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLHasSideEffects.cpp"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLIsConstantExpression.cpp"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLIsDynamicallyUniformExpression.cpp"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLIsSameExpressionTree.cpp"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLIsTrivialExpression.cpp"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLProgramUsage.cpp"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLReturnsInputAlpha.cpp"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLSpecialization.cpp"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLSwitchCaseContainsExit.cpp"
        "${SKIA_ROOT}/src/sksl/analysis/SkSLSymbolTableStackBuilder.cpp"
        "${SKIA_ROOT}/src/sksl/codegen/SkSLGLSLCodeGenerator.cpp"
        "${SKIA_ROOT}/src/sksl/codegen/SkSLMetalCodeGenerator.cpp"
        "${SKIA_ROOT}/src/sksl/codegen/SkSLPipelineStageCodeGenerator.cpp"
        "${SKIA_ROOT}/src/sksl/codegen/SkSLRasterPipelineBuilder.cpp"
        "${SKIA_ROOT}/src/sksl/codegen/SkSLRasterPipelineCodeGenerator.cpp"
        "${SKIA_ROOT}/src/sksl/codegen/SkSLSPIRVCodeGenerator.cpp"
        "${SKIA_ROOT}/src/sksl/codegen/SkSLWGSLCodeGenerator.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLBinaryExpression.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLBlock.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLChildCall.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructor.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorArray.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorArrayCast.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorCompound.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorCompoundCast.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorDiagonalMatrix.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorMatrixResize.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorScalarCast.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorSplat.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLConstructorStruct.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLDiscardStatement.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLDoStatement.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLExpression.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLExpressionStatement.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLExtension.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLFieldAccess.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLForStatement.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLFunctionCall.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLFunctionDeclaration.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLFunctionDefinition.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLIfStatement.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLIndexExpression.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLInterfaceBlock.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLLayout.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLLiteral.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLModifierFlags.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLModifiersDeclaration.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLPostfixExpression.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLPrefixExpression.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLProgram.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLSetting.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLStructDefinition.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLSwitchCase.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLSwitchStatement.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLSwizzle.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLSymbol.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLSymbolTable.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLTernaryExpression.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLType.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLTypeReference.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLVarDeclarations.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLVariable.cpp"
        "${SKIA_ROOT}/src/sksl/ir/SkSLVariableReference.cpp"
        "${SKIA_ROOT}/src/sksl/tracing/SkSLDebugTracePlayer.cpp"
        "${SKIA_ROOT}/src/sksl/tracing/SkSLDebugTracePriv.cpp"
        "${SKIA_ROOT}/src/sksl/tracing/SkSLTraceHook.cpp"
        "${SKIA_ROOT}/src/sksl/transform/SkSLAddConstToVarModifiers.cpp"
        "${SKIA_ROOT}/src/sksl/transform/SkSLEliminateDeadFunctions.cpp"
        "${SKIA_ROOT}/src/sksl/transform/SkSLEliminateDeadGlobalVariables.cpp"
        "${SKIA_ROOT}/src/sksl/transform/SkSLEliminateDeadLocalVariables.cpp"
        "${SKIA_ROOT}/src/sksl/transform/SkSLEliminateEmptyStatements.cpp"
        "${SKIA_ROOT}/src/sksl/transform/SkSLEliminateUnnecessaryBraces.cpp"
        "${SKIA_ROOT}/src/sksl/transform/SkSLEliminateUnreachableCode.cpp"
        "${SKIA_ROOT}/src/sksl/transform/SkSLFindAndDeclareBuiltinFunctions.cpp"
        "${SKIA_ROOT}/src/sksl/transform/SkSLFindAndDeclareBuiltinStructs.cpp"
        "${SKIA_ROOT}/src/sksl/transform/SkSLFindAndDeclareBuiltinVariables.cpp"
        "${SKIA_ROOT}/src/sksl/transform/SkSLHoistSwitchVarDeclarationsAtTopLevel.cpp"
        "${SKIA_ROOT}/src/sksl/transform/SkSLRenamePrivateSymbols.cpp"
        "${SKIA_ROOT}/src/sksl/transform/SkSLReplaceConstVarsWithLiterals.cpp"
        "${SKIA_ROOT}/src/sksl/transform/SkSLReplaceSplatCastsWithSwizzles.cpp"
        "${SKIA_ROOT}/src/sksl/transform/SkSLRewriteIndexedSwizzle.cpp"
        "${SKIA_ROOT}/src/sksl/transform/SkSLTransform.cpp"
    )

    set(SKIA_PORT_HEADERS
        "${SKIA_ROOT}/src/ports/SkScalerContext_win_dw.h"
        "${SKIA_ROOT}/src/ports/SkTypeface_win_dw.h"
    )

    set(SKIA_PORT_SOURCES
        "${SKIA_ROOT}/src/ports/SkMemory_malloc.cpp"
        "${SKIA_ROOT}/src/ports/SkGlobalInitialization_default.cpp"
        "${SKIA_ROOT}/src/ports/SkOSFile_stdio.cpp"
        "${SKIA_ROOT}/src/sfnt/SkOTUtils.cpp"
        "${SKIA_ROOT}/src/sfnt/SkOTTable_name.cpp"
    )

    # Platform-specific system font manager (non-FreeType)
    if(WIN32)
        list(APPEND SKIA_PORT_SOURCES
            "${SKIA_ROOT}/src/ports/SkFontMgr_win_dw.cpp"
            "${SKIA_ROOT}/src/ports/SkOSFile_win.cpp"
            "${SKIA_ROOT}/src/ports/SkScalerContext_win_dw.cpp"
            "${SKIA_ROOT}/src/ports/SkTypeface_win_dw.cpp"
            "${SKIA_ROOT}/src/ports/SkImageGeneratorWIC.cpp"
            "${SKIA_ROOT}/src/ports/SkLog_win.cpp"
        )
    endif()

    # Non-Windows: POSIX file ops and stdio logging
    if(NOT WIN32)
        list(APPEND SKIA_PORT_SOURCES
            "${SKIA_ROOT}/src/ports/SkOSFile_posix.cpp"
            "${SKIA_ROOT}/src/ports/SkLog_stdio.cpp"
        )
    endif()

    # FreeType support sources (cross-platform font rendering)
    if(SKIA_USE_FREETYPE)
        list(APPEND SKIA_PORT_SOURCES
            "${SKIA_ROOT}/src/ports/SkFontHost_FreeType.cpp"
            "${SKIA_ROOT}/src/ports/SkFontHost_FreeType_common.cpp"
            "${SKIA_ROOT}/src/ports/SkFontMgr_custom.cpp"
            "${SKIA_ROOT}/src/ports/SkFontMgr_custom_directory.cpp"
            "${SKIA_ROOT}/src/ports/SkFontMgr_custom_embedded.cpp"
            "${SKIA_ROOT}/src/ports/SkFontMgr_custom_empty.cpp"
        )
        list(APPEND SKIA_PORT_HEADERS
            "${SKIA_ROOT}/src/ports/SkFontHost_FreeType_common.h"
            "${SKIA_ROOT}/src/ports/SkFontScanner_FreeType_priv.h"
            "${SKIA_ROOT}/src/ports/SkTypeface_FreeType.h"
            "${SKIA_ROOT}/src/ports/SkFontMgr_custom.h"
        )
    endif()

    # FontConfig (Linux system font discovery)
    if(FONTCONFIG_FOUND)
        list(APPEND SKIA_PORT_SOURCES
            "${SKIA_ROOT}/src/ports/SkFontMgr_fontconfig.cpp"
            "${SKIA_ROOT}/src/ports/SkTypeface_proxy.cpp"
        )
    endif()

    set(SKIA_PATHOPS_HEADERS
        "${SKIA_ROOT}/src/pathops/SkAddIntersections.h"
        "${SKIA_ROOT}/src/pathops/SkIntersectionHelper.h"
        "${SKIA_ROOT}/src/pathops/SkIntersections.h"
        "${SKIA_ROOT}/src/pathops/SkLineParameters.h"
        "${SKIA_ROOT}/src/pathops/SkOpAngle.h"
        "${SKIA_ROOT}/src/pathops/SkOpCoincidence.h"
        "${SKIA_ROOT}/src/pathops/SkOpContour.h"
        "${SKIA_ROOT}/src/pathops/SkOpEdgeBuilder.h"
        "${SKIA_ROOT}/src/pathops/SkOpSegment.h"
        "${SKIA_ROOT}/src/pathops/SkOpSpan.h"
        "${SKIA_ROOT}/src/pathops/SkPathOpsBounds.h"
        "${SKIA_ROOT}/src/pathops/SkPathOpsCommon.h"
        "${SKIA_ROOT}/src/pathops/SkPathOpsConic.h"
        "${SKIA_ROOT}/src/pathops/SkPathOpsCubic.h"
        "${SKIA_ROOT}/src/pathops/SkPathOpsCurve.h"
        "${SKIA_ROOT}/src/pathops/SkPathOpsDebug.h"
        "${SKIA_ROOT}/src/pathops/SkPathOpsLine.h"
        "${SKIA_ROOT}/src/pathops/SkPathOpsPoint.h"
        "${SKIA_ROOT}/src/pathops/SkPathOpsQuad.h"
        "${SKIA_ROOT}/src/pathops/SkPathOpsRect.h"
        "${SKIA_ROOT}/src/pathops/SkPathOpsTCurve.h"
        "${SKIA_ROOT}/src/pathops/SkPathOpsTSect.h"
        "${SKIA_ROOT}/src/pathops/SkPathOpsTypes.h"
        "${SKIA_ROOT}/src/pathops/SkPathWriter.h"
        "${SKIA_ROOT}/src/pathops/SkReduceOrder.h"
    )

    set(SKIA_PATHOPS_SOURCES
        "${SKIA_ROOT}/src/pathops/SkAddIntersections.cpp"
        "${SKIA_ROOT}/src/pathops/SkDConicLineIntersection.cpp"
        "${SKIA_ROOT}/src/pathops/SkDCubicLineIntersection.cpp"
        "${SKIA_ROOT}/src/pathops/SkDCubicToQuads.cpp"
        "${SKIA_ROOT}/src/pathops/SkDLineIntersection.cpp"
        "${SKIA_ROOT}/src/pathops/SkDQuadLineIntersection.cpp"
        "${SKIA_ROOT}/src/pathops/SkIntersections.cpp"
        "${SKIA_ROOT}/src/pathops/SkOpAngle.cpp"
        "${SKIA_ROOT}/src/pathops/SkOpBuilder.cpp"
        "${SKIA_ROOT}/src/pathops/SkOpCoincidence.cpp"
        "${SKIA_ROOT}/src/pathops/SkOpContour.cpp"
        "${SKIA_ROOT}/src/pathops/SkOpCubicHull.cpp"
        "${SKIA_ROOT}/src/pathops/SkOpEdgeBuilder.cpp"
        "${SKIA_ROOT}/src/pathops/SkOpSegment.cpp"
        "${SKIA_ROOT}/src/pathops/SkOpSpan.cpp"
        "${SKIA_ROOT}/src/pathops/SkPathOpsAsWinding.cpp"
        "${SKIA_ROOT}/src/pathops/SkPathOpsCommon.cpp"
        "${SKIA_ROOT}/src/pathops/SkPathOpsConic.cpp"
        "${SKIA_ROOT}/src/pathops/SkPathOpsCubic.cpp"
        "${SKIA_ROOT}/src/pathops/SkPathOpsCurve.cpp"
        "${SKIA_ROOT}/src/pathops/SkPathOpsDebug.cpp"
        "${SKIA_ROOT}/src/pathops/SkPathOpsLine.cpp"
        "${SKIA_ROOT}/src/pathops/SkPathOpsOp.cpp"
        "${SKIA_ROOT}/src/pathops/SkPathOpsQuad.cpp"
        "${SKIA_ROOT}/src/pathops/SkPathOpsRect.cpp"
        "${SKIA_ROOT}/src/pathops/SkPathOpsSimplify.cpp"
        "${SKIA_ROOT}/src/pathops/SkPathOpsTSect.cpp"
        "${SKIA_ROOT}/src/pathops/SkPathOpsTightBounds.cpp"
        "${SKIA_ROOT}/src/pathops/SkPathOpsTypes.cpp"
        "${SKIA_ROOT}/src/pathops/SkPathOpsWinding.cpp"
        "${SKIA_ROOT}/src/pathops/SkPathWriter.cpp"
        "${SKIA_ROOT}/src/pathops/SkReduceOrder.cpp"
    )

    set(SKIA_UTILS_HEADERS
        "${SKIA_ROOT}/src/utils/SkBitSet.h"
        "${SKIA_ROOT}/src/utils/SkCallableTraits.h"
        "${SKIA_ROOT}/src/utils/SkCanvasStack.h"
        "${SKIA_ROOT}/src/utils/SkCharToGlyphCache.h"
        "${SKIA_ROOT}/src/utils/SkClipStackUtils.h"
        "${SKIA_ROOT}/src/utils/SkDashPathPriv.h"
        "${SKIA_ROOT}/src/utils/SkFloatToDecimal.h"
        "${SKIA_ROOT}/src/utils/SkFloatUtils.h"
        "${SKIA_ROOT}/src/utils/SkJSONWriter.h"
        "${SKIA_ROOT}/src/utils/SkMatrix22.h"
        "${SKIA_ROOT}/src/utils/SkMultiPictureDocumentPriv.h"
        "${SKIA_ROOT}/src/utils/SkOSPath.h"
        "${SKIA_ROOT}/src/utils/SkPatchUtils.h"
        "${SKIA_ROOT}/src/utils/SkPolyUtils.h"
        "${SKIA_ROOT}/src/utils/SkShaderUtils.h"
        "${SKIA_ROOT}/src/utils/SkShadowTessellator.h"
        "${SKIA_ROOT}/src/utils/mac/SkCGBase.h"
        "${SKIA_ROOT}/src/utils/mac/SkCGGeometry.h"
        "${SKIA_ROOT}/src/utils/mac/SkCTFont.h"
        "${SKIA_ROOT}/src/utils/mac/SkCTFontCreateExactCopy.h"
        "${SKIA_ROOT}/src/utils/mac/SkUniqueCFRef.h"
        "${SKIA_ROOT}/src/utils/win/SkAutoCoInitialize.h"
        "${SKIA_ROOT}/src/utils/win/SkDWrite.h"
        "${SKIA_ROOT}/src/utils/win/SkDWriteFontFileStream.h"
        "${SKIA_ROOT}/src/utils/win/SkDWriteGeometrySink.h"
        "${SKIA_ROOT}/src/utils/win/SkDWriteNTDDI_VERSION.h"
        "${SKIA_ROOT}/src/utils/win/SkHRESULT.h"
        "${SKIA_ROOT}/src/utils/win/SkIStream.h"
        "${SKIA_ROOT}/src/utils/win/SkObjBase.h"
        "${SKIA_ROOT}/src/utils/win/SkTScopedComPtr.h"
    )

    set(SKIA_UTILS_SOURCES
        "${SKIA_ROOT}/src/utils/SkCamera.cpp"
        "${SKIA_ROOT}/src/utils/SkCanvasStack.cpp"
        "${SKIA_ROOT}/src/utils/SkCanvasStateUtils.cpp"
        "${SKIA_ROOT}/src/utils/SkCharToGlyphCache.cpp"
        "${SKIA_ROOT}/src/utils/SkClipStackUtils.cpp"
        "${SKIA_ROOT}/src/utils/SkCustomTypeface.cpp"
        "${SKIA_ROOT}/src/utils/SkDashPath.cpp"
        "${SKIA_ROOT}/src/utils/SkEventTracer.cpp"
        "${SKIA_ROOT}/src/utils/SkFloatToDecimal.cpp"
        "${SKIA_ROOT}/src/utils/SkJSONWriter.cpp"
        "${SKIA_ROOT}/src/utils/SkMatrix22.cpp"
        "${SKIA_ROOT}/src/utils/SkMultiPictureDocument.cpp"
        "${SKIA_ROOT}/src/utils/SkNWayCanvas.cpp"
        "${SKIA_ROOT}/src/utils/SkNullCanvas.cpp"
        "${SKIA_ROOT}/src/utils/SkOSPath.cpp"
        "${SKIA_ROOT}/src/utils/SkOrderedFontMgr.cpp"
        "${SKIA_ROOT}/src/utils/SkPaintFilterCanvas.cpp"
        "${SKIA_ROOT}/src/utils/SkParse.cpp"
        "${SKIA_ROOT}/src/utils/SkParseColor.cpp"
        "${SKIA_ROOT}/src/utils/SkParsePath.cpp"
        "${SKIA_ROOT}/src/utils/SkPatchUtils.cpp"
        "${SKIA_ROOT}/src/utils/SkPolyUtils.cpp"
        "${SKIA_ROOT}/src/utils/SkShaderUtils.cpp"
        "${SKIA_ROOT}/src/utils/SkShadowTessellator.cpp"
        "${SKIA_ROOT}/src/utils/SkShadowUtils.cpp"
        "${SKIA_ROOT}/src/utils/SkTextUtils.cpp"
        "${SKIA_ROOT}/src/utils/mac/SkCTFont.cpp"
        "${SKIA_ROOT}/src/utils/mac/SkCTFontCreateExactCopy.cpp"
        "${SKIA_ROOT}/src/utils/mac/SkCreateCGImageRef.cpp"
        "${SKIA_ROOT}/src/utils/win/SkAutoCoInitialize.cpp"
        "${SKIA_ROOT}/src/utils/win/SkDWrite.cpp"
        "${SKIA_ROOT}/src/utils/win/SkDWriteFontFileStream.cpp"
        "${SKIA_ROOT}/src/utils/win/SkDWriteGeometrySink.cpp"
        "${SKIA_ROOT}/src/utils/win/SkHRESULT.cpp"
        "${SKIA_ROOT}/src/utils/win/SkIStream.cpp"
    )

    set(SKIA_XML_HEADERS
        "${SKIA_ROOT}/src/xml/SkDOM.h"
        "${SKIA_ROOT}/src/xml/SkXMLParser.h"
        "${SKIA_ROOT}/src/xml/SkXMLWriter.h"
    )

    set(SKIA_XML_SOURCES
        "${SKIA_ROOT}/src/xml/SkDOM.cpp"
        "${SKIA_ROOT}/src/xml/SkXMLParser.cpp"
        "${SKIA_ROOT}/src/xml/SkXMLWriter.cpp"
    )

    set(SKIA_SHAPER_HEADERS
        "${SKIA_ROOT}/modules/skshaper/include/SkShaper.h"
        "${SKIA_ROOT}/modules/skshaper/include/SkShaper_factory.h"
        "${SKIA_ROOT}/modules/skshaper/include/SkShaper_harfbuzz.h"
        "${SKIA_ROOT}/modules/skshaper/include/SkShaper_skunicode.h"
        "${SKIA_ROOT}/modules/skshaper/utils/FactoryHelpers.h"
    )

    set(SKIA_SHAPER_SOURCES
        "${SKIA_ROOT}/modules/skshaper/src/SkShaper.cpp"
        "${SKIA_ROOT}/modules/skshaper/src/SkShaper_factory.cpp"
        "${SKIA_ROOT}/modules/skshaper/src/SkShaper_primitive.cpp"
    )

    if(SKIA_USE_HARFBUZZ)
        list(APPEND SKIA_SHAPER_SOURCES
            "${SKIA_ROOT}/modules/skshaper/src/SkShaper_harfbuzz.cpp"
        )
    endif()

    if(SKIA_USE_ICU)
        list(APPEND SKIA_SHAPER_SOURCES
            "${SKIA_ROOT}/modules/skshaper/src/SkShaper_skunicode.cpp"
        )
    endif()

    set(SKIA_UNICODE_HEADERS
        "${SKIA_ROOT}/modules/skunicode/include/SkUnicode.h"
        "${SKIA_ROOT}/modules/skunicode/include/SkUnicode_bidi.h"
        "${SKIA_ROOT}/modules/skunicode/include/SkUnicode_client.h"
        "${SKIA_ROOT}/modules/skunicode/include/SkUnicode_icu.h"
        "${SKIA_ROOT}/modules/skunicode/include/SkUnicode_icu4x.h"
        "${SKIA_ROOT}/modules/skunicode/include/SkUnicode_libgrapheme.h"
        "${SKIA_ROOT}/modules/skunicode/src/SkBidiFactory_icu_full.h"
        "${SKIA_ROOT}/modules/skunicode/src/SkUnicode_hardcoded.h"
        "${SKIA_ROOT}/modules/skunicode/src/SkUnicode_icu_bidi.h"
        "${SKIA_ROOT}/modules/skunicode/src/SkUnicode_icupriv.h"
    )

    set(SKIA_UNICODE_SOURCES
        "${SKIA_ROOT}/modules/skunicode/src/SkBidiFactory_icu_full.cpp"
        "${SKIA_ROOT}/modules/skunicode/src/SkUnicode.cpp"
        "${SKIA_ROOT}/modules/skunicode/src/SkUnicode_hardcoded.cpp"
        "${SKIA_ROOT}/modules/skunicode/src/SkUnicode_icu.cpp"
        "${SKIA_ROOT}/modules/skunicode/src/SkUnicode_icu_bidi.cpp"
        "${SKIA_ROOT}/modules/skunicode/src/SkUnicode_icu_builtin.cpp"
    )

# ============================================================================
# Graphite GPU engine sources
# ============================================================================
file(GLOB_RECURSE SKIA_GRAPHITE_SOURCES
    "${SKIA_ROOT}/src/gpu/graphite/*.cpp"
)
# Remove backend-specific sources that need extra SDKs
list(FILTER SKIA_GRAPHITE_SOURCES EXCLUDE REGEX "/dawn/")
list(FILTER SKIA_GRAPHITE_SOURCES EXCLUDE REGEX "/mtl/")
# Remove Vello (needs Rust/CXX codegen step)
list(FILTER SKIA_GRAPHITE_SOURCES EXCLUDE REGEX "Vello")
# Remove Android platform specifics and sparse_strips
list(FILTER SKIA_GRAPHITE_SOURCES EXCLUDE REGEX "Android")
list(FILTER SKIA_GRAPHITE_SOURCES EXCLUDE REGEX "sparse_strips")
# Remove precompile (needs SPIRV-Tools)
list(FILTER SKIA_GRAPHITE_SOURCES EXCLUDE REGEX "precompile")
# Remove Vulkan-specific Graphite sources when VK backend is disabled
if(NOT SKIA_USE_VULKAN)
    list(FILTER SKIA_GRAPHITE_SOURCES EXCLUDE REGEX "/vk/")
endif()

# Add SkSL Graphite modules (needed by graphite shader compilation)
list(APPEND SKIA_SKSL_SOURCES
    "${SKIA_ROOT}/src/sksl/SkSLGraphiteModules.cpp"
)

# ---- Vulkan utility sources (needed by Graphite VK backend) ----
if(SKIA_USE_VULKAN)
    set(SKIA_VK_SOURCES
        "${SKIA_ROOT}/src/gpu/vk/VulkanMemory.cpp"
        "${SKIA_ROOT}/src/gpu/vk/VulkanInterface.cpp"
        "${SKIA_ROOT}/src/gpu/vk/VulkanExtensions.cpp"
        "${SKIA_ROOT}/src/gpu/vk/VulkanMutableTextureState.cpp"
        "${SKIA_ROOT}/src/gpu/vk/VulkanUtilsPriv.cpp"
        "${SKIA_ROOT}/src/gpu/vk/VulkanPreferredFeatures.cpp"
        "${SKIA_ROOT}/src/gpu/vk/vulkanmemoryallocator/VulkanMemoryAllocatorWrapper.cpp"
        "${SKIA_ROOT}/src/gpu/vk/vulkanmemoryallocator/VulkanAMDMemoryAllocator.cpp"
    )
    skia_filter_sources(SKIA_VK_SOURCES)
endif()

    # Add public graphit headers to public includes
list(INSERT SKIA_PUBLIC_HEADERS 0
        "${SKIA_ROOT}/include/gpu/graphite/Context.h"
        "${SKIA_ROOT}/include/gpu/graphite/ContextOptions.h"
        "${SKIA_ROOT}/include/gpu/graphite/GraphiteTypes.h"
        "${SKIA_ROOT}/include/gpu/graphite/Recorder.h"
        "${SKIA_ROOT}/include/gpu/graphite/Recording.h"
        "${SKIA_ROOT}/include/gpu/graphite/Surface.h"
        "${SKIA_ROOT}/include/gpu/graphite/Image.h"
        "${SKIA_ROOT}/include/gpu/graphite/BackendTexture.h"
        "${SKIA_ROOT}/include/gpu/graphite/BackendSemaphore.h"
        "${SKIA_ROOT}/include/gpu/graphite/TextureInfo.h"
        "${SKIA_ROOT}/include/gpu/graphite/YUVABackendTextures.h"
        "${SKIA_ROOT}/include/gpu/graphite/PrecompileContext.h"
        "${SKIA_ROOT}/include/gpu/graphite/PersistentPipelineStorage.h"
        "${SKIA_ROOT}/include/gpu/graphite/ImageProvider.h"
    )


# ============================================================================
# Public headers (header-only targets for IDE support)
# ============================================================================
set(SKIA_PUBLIC_HEADERS
    "${SKIA_ROOT}/include/codec/SkCodec.h"
    "${SKIA_ROOT}/include/codec/SkCodecAnimation.h"
    "${SKIA_ROOT}/include/codec/SkEncodedImageFormat.h"
    "${SKIA_ROOT}/include/codec/SkEncodedOrigin.h"
    "${SKIA_ROOT}/include/codec/SkPixmapUtils.h"
    "${SKIA_ROOT}/include/core/SkAlphaType.h"
    "${SKIA_ROOT}/include/core/SkAnnotation.h"
    "${SKIA_ROOT}/include/core/SkArc.h"
    "${SKIA_ROOT}/include/core/SkBBHFactory.h"
    "${SKIA_ROOT}/include/core/SkBitmap.h"
    "${SKIA_ROOT}/include/core/SkBlendMode.h"
    "${SKIA_ROOT}/include/core/SkBlender.h"
    "${SKIA_ROOT}/include/core/SkBlurTypes.h"
    "${SKIA_ROOT}/include/core/SkCPUContext.h"
    "${SKIA_ROOT}/include/core/SkCPURecorder.h"
    "${SKIA_ROOT}/include/core/SkCanvas.h"
    "${SKIA_ROOT}/include/core/SkCanvasVirtualEnforcer.h"
    "${SKIA_ROOT}/include/core/SkCapabilities.h"
    "${SKIA_ROOT}/include/core/SkClipOp.h"
    "${SKIA_ROOT}/include/core/SkColor.h"
    "${SKIA_ROOT}/include/core/SkColorFilter.h"
    "${SKIA_ROOT}/include/core/SkColorSpace.h"
    "${SKIA_ROOT}/include/core/SkColorTable.h"
    "${SKIA_ROOT}/include/core/SkColorType.h"
    "${SKIA_ROOT}/include/core/SkContourMeasure.h"
    "${SKIA_ROOT}/include/core/SkCoverageMode.h"
    "${SKIA_ROOT}/include/core/SkCubicMap.h"
    "${SKIA_ROOT}/include/core/SkData.h"
    "${SKIA_ROOT}/include/core/SkDataTable.h"
    "${SKIA_ROOT}/include/core/SkDocument.h"
    "${SKIA_ROOT}/include/core/SkDrawable.h"
    "${SKIA_ROOT}/include/core/SkExecutor.h"
    "${SKIA_ROOT}/include/core/SkFlattenable.h"
    "${SKIA_ROOT}/include/core/SkFont.h"
    "${SKIA_ROOT}/include/core/SkFontArguments.h"
    "${SKIA_ROOT}/include/core/SkFontMetrics.h"
    "${SKIA_ROOT}/include/core/SkFontMgr.h"
    "${SKIA_ROOT}/include/core/SkFontParameters.h"
    "${SKIA_ROOT}/include/core/SkFontScanner.h"
    "${SKIA_ROOT}/include/core/SkFontStyle.h"
    "${SKIA_ROOT}/include/core/SkFontTypes.h"
    "${SKIA_ROOT}/include/core/SkFourByteTag.h"
    "${SKIA_ROOT}/include/core/SkGraphics.h"
    "${SKIA_ROOT}/include/core/SkImage.h"
    "${SKIA_ROOT}/include/core/SkImageFilter.h"
    "${SKIA_ROOT}/include/core/SkImageGenerator.h"
    "${SKIA_ROOT}/include/core/SkImageInfo.h"
    "${SKIA_ROOT}/include/core/SkM44.h"
    "${SKIA_ROOT}/include/core/SkMallocPixelRef.h"
    "${SKIA_ROOT}/include/core/SkMaskFilter.h"
    "${SKIA_ROOT}/include/core/SkMatrix.h"
    "${SKIA_ROOT}/include/core/SkMesh.h"
    "${SKIA_ROOT}/include/core/SkMilestone.h"
    "${SKIA_ROOT}/include/core/SkOpenTypeSVGDecoder.h"
    "${SKIA_ROOT}/include/core/SkOverdrawCanvas.h"
    "${SKIA_ROOT}/include/core/SkPaint.h"
    "${SKIA_ROOT}/include/core/SkPath.h"
    "${SKIA_ROOT}/include/core/SkPathBuilder.h"
    "${SKIA_ROOT}/include/core/SkPathEffect.h"
    "${SKIA_ROOT}/include/core/SkPathIter.h"
    "${SKIA_ROOT}/include/core/SkPathMeasure.h"
    "${SKIA_ROOT}/include/core/SkPathTypes.h"
    "${SKIA_ROOT}/include/core/SkPathUtils.h"
    "${SKIA_ROOT}/include/core/SkPicture.h"
    "${SKIA_ROOT}/include/core/SkPictureRecorder.h"
    "${SKIA_ROOT}/include/core/SkPixelRef.h"
    "${SKIA_ROOT}/include/core/SkPixmap.h"
    "${SKIA_ROOT}/include/core/SkPoint.h"
    "${SKIA_ROOT}/include/core/SkPoint3.h"
    "${SKIA_ROOT}/include/core/SkRRect.h"
    "${SKIA_ROOT}/include/core/SkRSXform.h"
    "${SKIA_ROOT}/include/core/SkRasterHandleAllocator.h"
    "${SKIA_ROOT}/include/core/SkRecorder.h"
    "${SKIA_ROOT}/include/core/SkRect.h"
    "${SKIA_ROOT}/include/core/SkRefCnt.h"
    "${SKIA_ROOT}/include/core/SkRegion.h"
    "${SKIA_ROOT}/include/core/SkSamplingOptions.h"
    "${SKIA_ROOT}/include/core/SkScalar.h"
    "${SKIA_ROOT}/include/core/SkSerialProcs.h"
    "${SKIA_ROOT}/include/core/SkShader.h"
    "${SKIA_ROOT}/include/core/SkSize.h"
    "${SKIA_ROOT}/include/core/SkSpan.h"
    "${SKIA_ROOT}/include/core/SkStream.h"
    "${SKIA_ROOT}/include/core/SkStrikeRef.h"
    "${SKIA_ROOT}/include/core/SkString.h"
    "${SKIA_ROOT}/include/core/SkStrokeRec.h"
    "${SKIA_ROOT}/include/core/SkSurface.h"
    "${SKIA_ROOT}/include/core/SkSurfaceProps.h"
    "${SKIA_ROOT}/include/core/SkSwizzle.h"
    "${SKIA_ROOT}/include/core/SkTextBlob.h"
    "${SKIA_ROOT}/include/core/SkTextureCompressionType.h"
    "${SKIA_ROOT}/include/core/SkTileMode.h"
    "${SKIA_ROOT}/include/core/SkTiledImageUtils.h"
    "${SKIA_ROOT}/include/core/SkTraceMemoryDump.h"
    "${SKIA_ROOT}/include/core/SkTypeface.h"
    "${SKIA_ROOT}/include/core/SkTypes.h"
    "${SKIA_ROOT}/include/core/SkUnPreMultiply.h"
    "${SKIA_ROOT}/include/core/SkVertices.h"
    "${SKIA_ROOT}/include/core/SkYUVAInfo.h"
    "${SKIA_ROOT}/include/core/SkYUVAPixmaps.h"
    "${SKIA_ROOT}/include/effects/Sk1DPathEffect.h"
    "${SKIA_ROOT}/include/effects/Sk2DPathEffect.h"
    "${SKIA_ROOT}/include/effects/SkBlenders.h"
    "${SKIA_ROOT}/include/effects/SkBlurMaskFilter.h"
    "${SKIA_ROOT}/include/effects/SkColorMatrix.h"
    "${SKIA_ROOT}/include/effects/SkColorMatrixFilter.h"
    "${SKIA_ROOT}/include/effects/SkCornerPathEffect.h"
    "${SKIA_ROOT}/include/effects/SkDashPathEffect.h"
    "${SKIA_ROOT}/include/effects/SkDiscretePathEffect.h"
    "${SKIA_ROOT}/include/effects/SkGradient.h"
    "${SKIA_ROOT}/include/effects/SkHighContrastFilter.h"
    "${SKIA_ROOT}/include/effects/SkImageFilters.h"
    "${SKIA_ROOT}/include/effects/SkLumaColorFilter.h"
    "${SKIA_ROOT}/include/effects/SkOverdrawColorFilter.h"
    "${SKIA_ROOT}/include/effects/SkPerlinNoiseShader.h"
    "${SKIA_ROOT}/include/effects/SkRuntimeEffect.h"
    "${SKIA_ROOT}/include/effects/SkShaderMaskFilter.h"
    "${SKIA_ROOT}/include/effects/SkTableMaskFilter.h"
    "${SKIA_ROOT}/include/effects/SkTrimPathEffect.h"
    "${SKIA_ROOT}/include/gpu/GpuTypes.h"
    "${SKIA_ROOT}/include/gpu/MutableTextureState.h"
    "${SKIA_ROOT}/include/gpu/ShaderErrorHandler.h"
    "${SKIA_ROOT}/include/gpu/ganesh/GrBackendSemaphore.h"
    "${SKIA_ROOT}/include/gpu/ganesh/GrBackendSurface.h"
    "${SKIA_ROOT}/include/gpu/ganesh/GrContextOptions.h"
    "${SKIA_ROOT}/include/gpu/ganesh/GrContextThreadSafeProxy.h"
    "${SKIA_ROOT}/include/gpu/ganesh/GrDirectContext.h"
    "${SKIA_ROOT}/include/gpu/ganesh/GrDriverBugWorkarounds.h"
    "${SKIA_ROOT}/include/gpu/ganesh/GrDriverBugWorkaroundsAutogen.h"
    "${SKIA_ROOT}/include/gpu/ganesh/GrExternalTextureGenerator.h"
    "${SKIA_ROOT}/include/gpu/ganesh/GrRecordingContext.h"
    "${SKIA_ROOT}/include/gpu/ganesh/GrTypes.h"
    "${SKIA_ROOT}/include/gpu/ganesh/GrYUVABackendTextures.h"
    "${SKIA_ROOT}/include/gpu/ganesh/SkImageGanesh.h"
    "${SKIA_ROOT}/include/gpu/ganesh/SkMeshGanesh.h"
    "${SKIA_ROOT}/include/gpu/ganesh/SkSurfaceGanesh.h"
    "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLAssembleHelpers.h"
    "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLAssembleInterface.h"
    "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLBackendSurface.h"
    "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLConfig.h"
    "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLDirectContext.h"
    "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLExtensions.h"
    "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLFunctions.h"
    "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLInterface.h"
    "${SKIA_ROOT}/include/gpu/ganesh/gl/GrGLTypes.h"
    "${SKIA_ROOT}/include/gpu/ganesh/mock/GrMockBackendSurface.h"
    "${SKIA_ROOT}/include/gpu/ganesh/mock/GrMockTypes.h"
    "${SKIA_ROOT}/include/utils/SkCamera.h"
    "${SKIA_ROOT}/include/utils/SkCanvasStateUtils.h"
    "${SKIA_ROOT}/include/utils/SkCustomTypeface.h"
    "${SKIA_ROOT}/include/utils/SkEventTracer.h"
    "${SKIA_ROOT}/include/utils/SkNWayCanvas.h"
    "${SKIA_ROOT}/include/utils/SkNoDrawCanvas.h"
    "${SKIA_ROOT}/include/utils/SkNullCanvas.h"
    "${SKIA_ROOT}/include/utils/SkOrderedFontMgr.h"
    "${SKIA_ROOT}/include/utils/SkPaintFilterCanvas.h"
    "${SKIA_ROOT}/include/utils/SkParse.h"
    "${SKIA_ROOT}/include/utils/SkParsePath.h"
    "${SKIA_ROOT}/include/utils/SkShadowUtils.h"
    "${SKIA_ROOT}/include/utils/SkTextUtils.h"
    "${SKIA_ROOT}/include/utils/SkTraceEventPhase.h"
    "${SKIA_ROOT}/include/utils/mac/SkCGUtils.h"
    "${SKIA_ROOT}/modules/skshaper/include/SkShaper.h"
    "${SKIA_ROOT}/modules/skshaper/include/SkShaper_factory.h"
    "${SKIA_ROOT}/modules/skshaper/include/SkShaper_harfbuzz.h"
    "${SKIA_ROOT}/modules/skshaper/include/SkShaper_skunicode.h"
    "${SKIA_ROOT}/modules/skshaper/utils/FactoryHelpers.h"
    "${SKIA_ROOT}/modules/skunicode/include/SkUnicode.h"
    "${SKIA_ROOT}/modules/skunicode/include/SkUnicode_bidi.h"
    "${SKIA_ROOT}/modules/skunicode/include/SkUnicode_client.h"
    "${SKIA_ROOT}/modules/skunicode/include/SkUnicode_icu.h"
    "${SKIA_ROOT}/modules/skunicode/include/SkUnicode_icu4x.h"
    "${SKIA_ROOT}/modules/skunicode/include/SkUnicode_libgrapheme.h"
)

# FreeType-related public headers
if(SKIA_USE_FREETYPE)
    list(APPEND SKIA_PUBLIC_HEADERS
        "${SKIA_ROOT}/include/ports/SkFontMgr_directory.h"
        "${SKIA_ROOT}/include/ports/SkFontMgr_data.h"
        "${SKIA_ROOT}/include/ports/SkFontMgr_empty.h"
        "${SKIA_ROOT}/include/ports/SkFontScanner_FreeType.h"
    )
endif()

# ============================================================================
# Create the target: skia (always static)
# ============================================================================
add_library(skia STATIC)

target_include_directories(skia PUBLIC
    "${SKIA_ROOT}"
    "${SKIA_ROOT}/include"
    "${SKIA_ROOT}/include/config"
    "${SKIA_ROOT}/include/core"
    "${SKIA_ROOT}/include/codec"
    "${SKIA_ROOT}/include/effects"
    "${SKIA_ROOT}/include/encode"
    "${SKIA_ROOT}/include/gpu"
    "${SKIA_ROOT}/include/gpu/ganesh"
    "${SKIA_ROOT}/include/android"
    "${SKIA_ROOT}/include/gpu/ganesh/gl"
    "${SKIA_ROOT}/include/gpu/graphite"
    "${SKIA_ROOT}/include/pathops"
    "${SKIA_ROOT}/include/ports"
    "${SKIA_ROOT}/include/private"
    "${SKIA_ROOT}/include/private/base"
    "${SKIA_ROOT}/include/private/chromium"
    "${SKIA_ROOT}/include/sksl"
    "${SKIA_ROOT}/include/utils"
    "${SKIA_ROOT}/modules/skshaper/include"
    "${SKIA_ROOT}/modules/skunicode/include"
    "${SKIA_ROOT}/modules/skcms"
    "${SKIA_THIRD_DIR}/icu"
)
if(SKIA_USE_VULKAN)
    target_include_directories(skia PUBLIC
        "${SKIA_ROOT}/include/gpu/vk"
        "${SKIA_ROOT}/include/private/gpu/vk"
    )
    # glad2 provides Vulkan types via header hijacking
    target_link_libraries(skia PUBLIC cevg_glad)
endif()

target_include_directories(skia PRIVATE
    "${SKIA_ROOT}"
    "${SKIA_ROOT}/src/core"
    "${SKIA_ROOT}/src/gpu"
    "${SKIA_ROOT}/src/gpu/ganesh"
    "${SKIA_ROOT}/src/gpu/ganesh/gl"
    "${SKIA_ROOT}/src/gpu/ganesh/gl/builders"
    "${SKIA_ROOT}/src/gpu/ganesh/effects"
    "${SKIA_ROOT}/src/gpu/ganesh/geometry"
    "${SKIA_ROOT}/src/gpu/ganesh/glsl"
    "${SKIA_ROOT}/src/gpu/ganesh/gradients"
    "${SKIA_ROOT}/src/gpu/ganesh/image"
    "${SKIA_ROOT}/src/gpu/ganesh/ops"
    "${SKIA_ROOT}/src/gpu/ganesh/surface"
    "${SKIA_ROOT}/src/gpu/ganesh/tessellate"
    "${SKIA_ROOT}/src/gpu/ganesh/text"
    "${SKIA_ROOT}/src/gpu/tessellate"
    "${SKIA_ROOT}/src/sfnt"
    "${SKIA_ROOT}/src/sksl"
    "${SKIA_ROOT}/src/text"
    "${SKIA_ROOT}/src/text/gpu"
    "${SKIA_ROOT}/src/utils"
    "${SKIA_ROOT}/third_party"
    "${SKIA_ROOT}/src/gpu/graphite"
)
if(SKIA_USE_VULKAN)
    target_include_directories(skia PRIVATE
        "${SKIA_ROOT}/src/gpu/graphite/vk"
        "${SKIA_ROOT}/src/gpu/vk"
        "${SKIA_ROOT}/src/gpu/vk/vulkanmemoryallocator"
        "${CMAKE_CURRENT_SOURCE_DIR}/../vulkanmemoryallocator/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../glad/include"
    )
endif()

if(SKIA_USE_FREETYPE)
    target_include_directories(skia PRIVATE
        "${SKIA_THIRD_DIR}/freetype/include"
    )
    # FreeType generates ftconfig.h and ftoption.h into the build directory
    target_include_directories(skia PRIVATE
        "${CMAKE_BINARY_DIR}/_deps/freetype/include"
    )
endif()

target_compile_definitions(skia PUBLIC ${SKIA_DEFINES})

# Filter out non-existent sources (cross-version compatibility)
skia_filter_sources(SKIA_CORE_SOURCES)
skia_filter_sources(SKIA_EFFECTS_SOURCES)
skia_filter_sources(SKIA_COLORFILTER_SOURCES)
skia_filter_sources(SKIA_IMAGEFILTER_SOURCES)
skia_filter_sources(SKIA_CODEC_SOURCES)
skia_filter_sources(SKIA_ENCODE_SOURCES)
skia_filter_sources(SKIA_GPU_SOURCES)
skia_filter_sources(SKIA_SKSL_SOURCES)
skia_filter_sources(SKIA_PORT_SOURCES)
skia_filter_sources(SKIA_PATHOPS_SOURCES)
skia_filter_sources(SKIA_UTILS_SOURCES)
skia_filter_sources(SKIA_XML_SOURCES)
skia_filter_sources(SKIA_SHAPER_SOURCES)
skia_filter_sources(SKIA_UNICODE_SOURCES)
skia_filter_sources(SKIA_GRAPHITE_SOURCES)

# ---- skcms (color management, required by Skia core) ----
set(SKIA_SKCMS_SOURCES
    "${SKIA_ROOT}/modules/skcms/skcms.cc"
    "${SKIA_ROOT}/modules/skcms/src/skcms_TransformBaseline.cc"
    "${SKIA_ROOT}/modules/skcms/src/skcms_TransformHsw.cc"
    "${SKIA_ROOT}/modules/skcms/src/skcms_TransformSkx.cc"
)
skia_filter_sources(SKIA_SKCMS_SOURCES)

target_sources(skia PRIVATE
    ${SKIA_CORE_SOURCES}
    ${SKIA_EFFECTS_SOURCES}
    ${SKIA_COLORFILTER_SOURCES}
    ${SKIA_IMAGEFILTER_SOURCES}
    ${SKIA_CODEC_SOURCES}
    ${SKIA_ENCODE_SOURCES}
    ${SKIA_GPU_SOURCES}
    ${SKIA_SKSL_SOURCES}
    ${SKIA_PORT_SOURCES}
    ${SKIA_PATHOPS_SOURCES}
    ${SKIA_UTILS_SOURCES}
    ${SKIA_XML_SOURCES}
    ${SKIA_SHAPER_SOURCES}
    ${SKIA_UNICODE_SOURCES}
    ${SKIA_GRAPHITE_SOURCES}
    ${SKIA_SKCMS_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/SkSLSPIRVValidator_stub.cpp"
)
if(SKIA_USE_VULKAN)
    target_sources(skia PRIVATE ${SKIA_VK_SOURCES})
endif()

# ---- skcms SIMD variants: compile with appropriate ISA flags ----
if(MSVC)
    set_source_files_properties("${SKIA_ROOT}/modules/skcms/src/skcms_TransformHsw.cc"
        PROPERTIES COMPILE_FLAGS "/arch:AVX2")
    set_source_files_properties("${SKIA_ROOT}/modules/skcms/src/skcms_TransformSkx.cc"
        PROPERTIES COMPILE_FLAGS "/arch:AVX512")
else()
    set_source_files_properties("${SKIA_ROOT}/modules/skcms/src/skcms_TransformHsw.cc"
        PROPERTIES COMPILE_FLAGS "-mavx2 -mfma -mf16c")
    set_source_files_properties("${SKIA_ROOT}/modules/skcms/src/skcms_TransformSkx.cc"
        PROPERTIES COMPILE_FLAGS "-mavx512f -mavx512dq -mavx512vl -mfma -mf16c")
endif()

# ============================================================================
# Link libraries
# ============================================================================
if(WIN32)
    target_link_libraries(skia PUBLIC
        Ole32.lib
        OleAut32.lib
        FontSub.lib
        User32.lib
        Usp10.lib
        Gdi32.lib
    )
    # Vulkan symbols are loaded at runtime by glad2 — no vulkan-1.lib needed
endif()

# GL is not supported in this build

if(SKIA_USE_HARFBUZZ)
    target_link_libraries(skia PUBLIC skia_harfbuzz)
endif()

if(SKIA_USE_ICU)
    target_link_libraries(skia PUBLIC ICU::icuuc ICU::icuin ICU::icudt)
    target_include_directories(skia PUBLIC
        "${SKIA_THIRD_DIR}/icu/source/common"
        "${SKIA_THIRD_DIR}/icu/source/common/unicode"
        "${SKIA_THIRD_DIR}/icu/source/i18n"
        "${SKIA_THIRD_DIR}/icu/source/i18n/unicode"
    )
    # Copy ICU data file to output directory (required at runtime on Windows only)
    # Linux/macOS: icudtl.dat is embedded into the binary via .S + .incbin
    if(WIN32)
        set(ICU_DAT_FILE "${SKIA_THIRD_DIR}/icu/icudtl.dat")
        if(EXISTS "${ICU_DAT_FILE}")
            add_custom_command(TARGET skia POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${ICU_DAT_FILE}"
                    "${CMAKE_BINARY_DIR}/bin/icudtl.dat"
                COMMENT "Copying icudtl.dat to bin directory"
            )
        endif()
    endif()
endif()

target_link_libraries(skia PUBLIC skia_expat)
target_link_libraries(skia PUBLIC skia_zlib)

if(SKIA_USE_FREETYPE)
    target_link_libraries(skia PUBLIC skia_freetype)
    target_include_directories(skia PUBLIC
        "${SKIA_THIRD_DIR}/freetype/include"
        "${CMAKE_BINARY_DIR}/_deps/freetype/include"
    )
endif()

# FontConfig (Linux) — needed by SkFontMgr_fontconfig
if(FONTCONFIG_FOUND)
    target_link_libraries(skia PUBLIC ${FONTCONFIG_LIBRARIES})
    target_include_directories(skia PUBLIC ${FONTCONFIG_INCLUDE_DIRS})
endif()

# ============================================================================
# Compiler options — suppress all warnings for Skia (third-party code)
# ============================================================================
if(MSVC)
    target_compile_options(skia PRIVATE
        /W0
        /bigobj
        /utf-8
    )
    # Static CRT (/MT) for static builds
    set_property(TARGET skia PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(skia PRIVATE
        -w
    )
endif()

# ---- Suppress warnings for Skia's third-party dependencies too ----
set(_SKIA_THIRD_PARTY_TARGETS harfbuzz zlibstatic expat icuuc icuin icudt freetype)
foreach(_tgt ${_SKIA_THIRD_PARTY_TARGETS})
    if(TARGET ${_tgt})
        if(MSVC)
            target_compile_options(${_tgt} PRIVATE /W0)
        else()
            target_compile_options(${_tgt} PRIVATE -w)
        endif()
    endif()
endforeach()

# Set C++ standard
set_target_properties(skia PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

message(STATUS "Skia build configuration:")
message(STATUS "  Graphite engine: enabled")
message(STATUS "  Vulkan:         ${SKIA_USE_VULKAN}")
message(STATUS "  HarfBuzz:       ${SKIA_USE_HARFBUZZ}")
message(STATUS "  ICU:            ${SKIA_USE_ICU}")
message(STATUS "  FreeType:       ${SKIA_USE_FREETYPE}")
message(STATUS "  Build type:     Release (static)")
message(STATUS "  C++ standard:   20")
message(STATUS "")
message(STATUS "NOTE: You need to link against:")
message(STATUS "  - harfbuzz.lib (for HarfBuzz)")
message(STATUS "  - icuuc.lib, icuin.lib (for ICU)")
message(STATUS "  - zlib.lib (for PDF deflate)")
if(SKIA_USE_FREETYPE)
message(STATUS "  - freetype.lib (for FreeType font rendering)")
endif()