# ============================================================================
# icu/CMakeLists.txt
# ICU as a source library — use via add_subdirectory() directly.
#
# Provides targets (with ALIAS):
#   ICU::icudt  — stub data (required by icuuc)
#   ICU::icuuc  — common library
#   ICU::icuin  — i18n library (depends on icuuc)
#
# Usage in parent CMakeLists.txt:
#   add_subdirectory(path/to/icu)
#   target_link_libraries(myapp PRIVATE ICU::icuuc ICU::icuin)
#
# Cross-platform: Windows (MSVC), Linux, macOS.
# ============================================================================

cmake_minimum_required(VERSION 3.20)

project(icu
  LANGUAGES C CXX
  VERSION 78.1
)

# ICU headers require C++17 (auto template params, nested namespaces)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- MSVC: static CRT by default ----
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ============================================================================
# Options
# ============================================================================
option(BUILD_ICU_I18N "Build ICU i18n (icuin) library" ON)

# ============================================================================
# Source file lists
# ============================================================================
set(ICU_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/source")

# -- Stubdata --
set(STUBDATA_SRCS "${ICU_SRC_DIR}/stubdata/stubdata.cpp")

# -- Common (icuuc) --
set(COMMON_SRC_DIR "${ICU_SRC_DIR}/common")
set(COMMON_SRCS_TXT "${COMMON_SRC_DIR}/sources.txt")

if(EXISTS "${COMMON_SRCS_TXT}")
  file(READ "${COMMON_SRCS_TXT}" _raw)
  string(STRIP "${_raw}" _raw)
  string(REPLACE "\n" ";" _list "${_raw}")
  foreach(_src ${_list})
    string(STRIP "${_src}" _s)
    if(_s AND NOT "${_s}" MATCHES "^#")
      list(APPEND COMMON_SRCS "${COMMON_SRC_DIR}/${_s}")
    endif()
  endforeach()
else()
  message(FATAL_ERROR "Not found: ${COMMON_SRCS_TXT}")
endif()

# -- I18n (icuin) --
set(I18N_SRC_DIR "${ICU_SRC_DIR}/i18n")
set(I18N_SRCS_TXT "${I18N_SRC_DIR}/sources.txt")

if(EXISTS "${I18N_SRCS_TXT}")
  file(READ "${I18N_SRCS_TXT}" _raw)
  string(STRIP "${_raw}" _raw)
  string(REPLACE "\n" ";" _list "${_raw}")
  foreach(_src ${_list})
    string(STRIP "${_src}" _s)
    if(_s AND NOT "${_s}" MATCHES "^#")
      list(APPEND I18N_SRCS "${I18N_SRC_DIR}/${_s}")
    endif()
  endforeach()
else()
  message(FATAL_ERROR "Not found: ${I18N_SRCS_TXT}")
endif()

# ============================================================================
# Include directories (build-interface only — no install)
# ============================================================================
set(ICU_INCLUDE_DIRS
  "${ICU_SRC_DIR}/common"
  "${ICU_SRC_DIR}/common/unicode"
  "${ICU_SRC_DIR}/i18n"
  "${ICU_SRC_DIR}/i18n/unicode"
  "${ICU_SRC_DIR}/stubdata"
  "${ICU_SRC_DIR}"
)

# ============================================================================
# Compile definitions
# ============================================================================
set(ICU_BASE_DEFS
  U_ATTRIBUTE_DEPRECATED=
  U_DISABLE_RENAMING=1
)

# U_STATIC_IMPLEMENTATION must be PUBLIC so consumers also get it
set(ICU_STATIC_DEF U_STATIC_IMPLEMENTATION)

set(ICU_COMMON_DEFS
  ${ICU_BASE_DEFS}
  U_COMMON_IMPLEMENTATION
)

set(ICU_I18N_DEFS
  ${ICU_BASE_DEFS}
  U_I18N_IMPLEMENTATION
)

# Platform-specific additions
if(WIN32)
  list(APPEND ICU_COMMON_DEFS
    WIN32
    _CRT_SECURE_NO_DEPRECATE
    _CRT_NONSTDC_NO_DEPRECATE
    U_PLATFORM_USES_ONLY_WIN32_API=1
  )
  list(APPEND ICU_I18N_DEFS
    WIN32
    _CRT_SECURE_NO_DEPRECATE
    _CRT_NONSTDC_NO_DEPRECATE
  )
endif()

if(MSVC)
  list(APPEND ICU_COMMON_DEFS _CRT_SECURE_NO_WARNINGS)
  list(APPEND ICU_I18N_DEFS   _CRT_SECURE_NO_WARNINGS)
endif()

# ============================================================================
# Targets
# ============================================================================

# ---- icudt ----
# Windows: stubdata (empty shell) + runtime file loading via SkLoadICU
# Linux/macOS: embed real ICU data via .S + .incbin
if(WIN32)
    add_library(icudt STATIC ${STUBDATA_SRCS})
else()
    set(ICU_DAT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/icudtl.dat")
    set(ICU_DAT_S "${CMAKE_CURRENT_BINARY_DIR}/icudtl_dat.S")
    file(WRITE "${ICU_DAT_S}" [=[.globl icudt74_dat
.section .rodata
icudt74_dat:
    .incbin "]=] "${ICU_DAT_FILE}" [=["
.size icudt74_dat, . - icudt74_dat
.section .note.GNU-stack,"",@progbits
]=])
    add_library(icudt STATIC "${ICU_DAT_S}")
endif()
target_include_directories(icudt
  PUBLIC
    "$<BUILD_INTERFACE:${ICU_SRC_DIR}/common>"
    "$<BUILD_INTERFACE:${ICU_SRC_DIR}/common/unicode>"
    "$<BUILD_INTERFACE:${ICU_SRC_DIR}/stubdata>"
    "$<BUILD_INTERFACE:${ICU_SRC_DIR}>"
)
target_compile_definitions(icudt
  PUBLIC ${ICU_STATIC_DEF}
  PRIVATE ${ICU_BASE_DEFS}
)
add_library(ICU::icudt ALIAS icudt)
set_target_properties(icudt PROPERTIES
  OUTPUT_NAME "icudt"
  POSITION_INDEPENDENT_CODE ON
)

# ---- icuuc (common) ----
# SkLoadICU.cpp only needed on Windows (runtime file loading)
if(WIN32)
    set(ICUUC_EXTRA_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/SkLoadICU.cpp")
else()
    set(ICUUC_EXTRA_SRCS)
endif()
add_library(icuuc STATIC ${COMMON_SRCS} ${ICUUC_EXTRA_SRCS})
target_include_directories(icuuc
  PUBLIC
    "$<BUILD_INTERFACE:${ICU_SRC_DIR}/common>"
    "$<BUILD_INTERFACE:${ICU_SRC_DIR}/common/unicode>"
  PRIVATE
    ${ICU_INCLUDE_DIRS}
)
target_compile_definitions(icuuc
  PUBLIC ${ICU_STATIC_DEF}
  PRIVATE ${ICU_COMMON_DEFS} SK_USING_THIRD_PARTY_ICU
)
target_link_libraries(icuuc PRIVATE icudt)
add_library(ICU::icuuc ALIAS icuuc)
set_target_properties(icuuc PROPERTIES
  OUTPUT_NAME "icuuc"
  POSITION_INDEPENDENT_CODE ON
)

# MSVC warning suppressions
if(MSVC)
  target_compile_options(icuuc PRIVATE /wd4996 /wd4267 /wd4244 /wd4018 /wd4305)
endif()

# ---- icuin (i18n) ----
if(BUILD_ICU_I18N)
  add_library(icuin STATIC ${I18N_SRCS})
  target_include_directories(icuin
    PUBLIC
      "$<BUILD_INTERFACE:${ICU_SRC_DIR}/i18n>"
      "$<BUILD_INTERFACE:${ICU_SRC_DIR}/i18n/unicode>"
      "$<BUILD_INTERFACE:${ICU_SRC_DIR}/common>"
      "$<BUILD_INTERFACE:${ICU_SRC_DIR}/common/unicode>"
    PRIVATE
      ${ICU_INCLUDE_DIRS}
  )
  target_compile_definitions(icuin
    PUBLIC ${ICU_STATIC_DEF}
    PRIVATE ${ICU_I18N_DEFS}
  )
  target_link_libraries(icuin PUBLIC icuuc)
  add_library(ICU::icuin ALIAS icuin)
  set_target_properties(icuin PROPERTIES
    OUTPUT_NAME "icuin"
    POSITION_INDEPENDENT_CODE ON
  )

  if(MSVC)
    target_compile_options(icuin PRIVATE /wd4996 /wd4267 /wd4244 /wd4018 /wd4305 /wd4805)
  endif()
endif()

# ============================================================================
# Summary
# ============================================================================
list(LENGTH COMMON_SRCS _ncommon)
list(LENGTH I18N_SRCS _ni18n)
message(STATUS "ICU ${PROJECT_VERSION} — source library")
message(STATUS "  icudt (stubdata)  static")
message(STATUS "  icuuc (common)    static  (${_ncommon} sources)")
message(STATUS "  icuin (i18n)      static  (${_ni18n} sources)")
message(STATUS "  Targets: ICU::icudt, ICU::icuuc, ICU::icuin")