#------------------------------------------------------------------------------
# FILE         :  CMakeLists.txt
#
# COPYRIGHT    :  Yakov Markovitch, 2014-2016
#                 See LICENSE for information on usage/redistribution.
#
# DESCRIPTION  :  CMake script for building pcommon library
#
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  29 Aug 2014
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5.2)

project(pcommon C CXX)

# We have some custom .cmake scripts in pcommon/config
list(APPEND CMAKE_MODULE_PATH "${pcommon_SOURCE_DIR}/config")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} CACHE PATH "")

include(pcommon)

message("***** Processing ${PROJECT_NAME} *****")

# Platform sources subdirectory
if (UNIX)
  set(PLATFORM_SOURCES unix)
else()
  set(PLATFORM_SOURCES win32)
endif(UNIX)

add_library(pcommon STATIC

  regex.c

  pcomn_crc32.c
  pcomn_hash.c

  pcomn_trace.cpp
  pcomn_tracecfg.cpp

  pcomn_binascii.cpp
  pcomn_binstream.cpp
  pcomn_cfgparser.cpp
  pcomn_diag.cpp
  pcomn_exec.cpp
  pcomn_fileutils.cpp

  pcomn_mmap.cpp
  pcomn_path.cpp
  pcomn_rawstream.cpp
  pcomn_regex.cpp
  pcomn_strsubst.cpp
  pcomn_textio.cpp
  pcomn_uri.cpp
  pcomn_uuid.cpp

  pcomn_ziowrap.c
  pcomn_zstream.cpp

  # Platform-dependent target sources
  ${PLATFORM_SOURCES}/pcomn_sys.cpp

  $<$<NOT:$<PLATFORM_ID:Windows>>:
  pcomn_crypthash
  unix/pcomn_posix_exec.cpp
  unix/pcomn_shutil.cpp>
)

# At least C++11
target_compile_features(pcommon
  PUBLIC
  cxx_alignas
  cxx_alignof
  cxx_delegating_constructors
  )

set_target_properties(pcommon
  PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  VISIBILITY_INLINES_HIDDEN ON
  CXX_VISIBILITY_PRESET hidden
  )

target_compile_features(pcommon
  PUBLIC
  cxx_alignas
  cxx_alignof
  cxx_constexpr
  cxx_decltype
  )

target_include_directories(pcommon
  PUBLIC
  ${PROJECT_SOURCE_DIR}
  )

target_compile_definitions(pcommon
  PUBLIC
  $<$<CONFIG:Debug>:${PCOMN_CHECK} ${PCOMN_TRACE}>
  )

target_compile_options(pcommon
  PUBLIC
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-pthread>
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wall>
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-gdwarf-4>
  $<$<CXX_COMPILER_ID:GNU>:-fno-debug-types-section -grecord-gcc-switches>
  $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-Wno-potentially-evaluated-expression -Wno-mismatched-tags>

  PRIVATE
  $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-Woverloaded-virtual>
  )

target_link_libraries(pcommon
  PUBLIC
  $<$<OR:$<PLATFORM_ID:Linux>,$<PLATFORM_ID:Cygwin>,$<PLATFORM_ID:MinGW>>:-pthread>
  )

add_subdirectory(unittests EXCLUDE_FROM_ALL)
