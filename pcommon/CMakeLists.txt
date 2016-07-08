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

force_flags(CMAKE_C_FLAGS ${PCOMN_C_OPT_BASE})
force_flags(CMAKE_CXX_FLAGS ${PCOMN_CXX_OPT_BASE})

add_library(pcommon STATIC

  regex.c

  pcomn_crc32.c
  pcomn_hash.c
  pcomn_version.c

  pcomn_trace.cpp
  pcomn_tracecfg.cpp

  pcomn_binascii.cpp
  pcomn_binstream.cpp
  pcomn_cfgparser.cpp
  pcomn_diag.cpp
  pcomn_exec.cpp
  pcomn_fileutils.cpp
  pcomn_incdlist.cpp

  pcomn_mmap.cpp
  pcomn_path.cpp
  pcomn_qualname.cpp
  pcomn_rawstream.cpp
  pcomn_regex.cpp
  pcomn_strsubst.cpp
  pcomn_textio.cpp
  pcomn_thread.cpp
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
set_target_properties(pcommon
  PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED ON
  POSITION_INDEPENDENT_CODE ON
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
  $<$<CONFIG:Debug>:${PCOMN_C_OPT_DBGINFO}>
  )

add_subdirectory(unittests EXCLUDE_FROM_ALL)

#print_all_variables()
