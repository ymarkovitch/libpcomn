#------------------------------------------------------------------------------
# FILE         :  CMakeLists.txt
#
# COPYRIGHT    :  Yakov Markovitch, 2014-2017
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

set(UNITTESTS ON CACHE BOOL "Enable unittests building. Requires CPPUntit.")

message("***** Processing ${PROJECT_NAME} *****")

if (NOT UNIX)
  if (NOT ZLIB_INCLUDE_DIR)
    find_package(ZLIB)
  endif()
  if (NOT ZLIB_INCLUDE_DIR)
    messsage(FATAL_ERROR "ZLib include path is not found. Please specify -DZLIB_INCLUDE_DIR manually.")
  endif()
else()
  find_package(OpenSSL)
  if (NOT OPENSSL_INCLUDE_DIR)
    message(FATAL_ERROR "OpenSSL library include files are not found. Please install OpenSSL development library.")
  endif()
endif()

if (${CMAKE_GENERATOR} MATCHES ".*Visual Studio.*")

  if (NOT "${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64")
    message(FATAL_ERROR "32-bit compilation is not supported. Please specify -DCMAKE_GENERATOR_PLATFORM=x64")
  endif()

  set(COMPILE_LANGUAGE_CXX "1:1")
else()
  set(COMPILE_LANGUAGE_CXX "COMPILE_LANGUAGE:CXX")
endif()

# Platform sources subdirectory
if (UNIX)
  set(PLATFORM_SOURCES unix)
else()
  set(PLATFORM_SOURCES win32)
endif(UNIX)

set(T1HA_SOURCES t1ha/src)

add_library(pcommon STATIC

  regex.c

  pcomn_crc32.c

  ${T1HA_SOURCES}/t1ha0.c
  ${T1HA_SOURCES}/t1ha1.c
  ${T1HA_SOURCES}/t1ha0_ia32aes_avx.c
  ${T1HA_SOURCES}/t1ha0_ia32aes_noavx.c

  pcomn_trace.cpp
  pcomn_tracecfg.cpp

  pcomn_binascii.cpp
  pcomn_binstream.cpp
  pcomn_cfgparser.cpp
  pcomn_diag.cpp
  pcomn_exec.cpp
  pcomn_fileutils.cpp
  pcomn_fileread.cpp

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

  $<$<NOT:$<PLATFORM_ID:Windows>>:
  pcomn_crypthash
  unix/pcomn_posix_exec.cpp
  unix/pcomn_shutil.cpp
  #unix/pcomn_stacktrace.cpp
  >
)

# At least C++11
target_compile_features(pcommon
  PUBLIC
  cxx_alignas
  cxx_alignof
  cxx_constexpr
  cxx_decltype
  cxx_delegating_constructors
  )

set_target_properties(pcommon
  PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  VISIBILITY_INLINES_HIDDEN ON
  CXX_VISIBILITY_PRESET hidden
  )

target_include_directories(pcommon
  PUBLIC
  ${PROJECT_SOURCE_DIR}
  ${ZLIB_INCLUDE_DIR}
  )

target_compile_definitions(pcommon
  PUBLIC
  $<$<OR:$<CONFIG:Debug>,$<CONFIG:DbgSanitize>>:${PCOMN_CHECK} ${PCOMN_TRACE}>
  $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x0600>

  PRIVATE
  $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN=1 UNICODE _UNICODE>
  )

target_compile_options(pcommon
  PUBLIC
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-pthread>
  $<$<CXX_COMPILER_ID:GNU>:-frecord-gcc-switches>

  PRIVATE
  $<$<CXX_COMPILER_ID:GNU>:-Wall -Wno-error>
  $<$<CXX_COMPILER_ID:Clang>:-Wall -Wno-error -Wno-mismatched-tags>
  $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>,$<${COMPILE_LANGUAGE_CXX}>>:-Woverloaded-virtual>

  $<$<AND:$<OR:$<CONFIG:Debug>,$<CONFIG:DbgSanitize>,$<CONFIG:RelWithDebInfo>>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-gdwarf-4>
  $<$<PLATFORM_ID:Linux>:-fvisibility=hidden>

  PUBLIC
  $<$<AND:$<CXX_COMPILER_ID:Clang>,$<${COMPILE_LANGUAGE_CXX}>>:
        -Wno-potentially-evaluated-expression -Wno-mismatched-tags
        -Wno-null-dereference -Wno-gnu>
  )

target_link_libraries(pcommon
  PUBLIC
  $<$<OR:$<PLATFORM_ID:Linux>,$<PLATFORM_ID:Cygwin>,$<PLATFORM_ID:MinGW>>:-pthread>
  )

if (${UNITTESTS})
  enable_testing()
  add_subdirectory(unittests EXCLUDE_FROM_ALL)
endif()
