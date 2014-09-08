#------------------------------------------------------------------------------
# FILE         :  CMakeLists.txt
#
# COPYRIGHT    :  Yakov Markovitch, 2014
#
# DESCRIPTION  :  CMake script for building pcommon library
#
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  29 Aug 2014
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.12)

project(pcommon C CXX)
# We have some custom .cmake scripts in pcommon/config
list(APPEND CMAKE_MODULE_PATH "${pcommon_SOURCE_DIR}/config")
include(pcommon)

message("***** Processing ${PROJECT_NAME} *****")

# Platform sources subdirectory
if (UNIX)
  set(PLATFORM_SOURCES unix)
else()
  set(PLATFORM_SOURCES win32)
endif(UNIX)

# Custom debug flags
set(PCOMN_DEBUG_FLAGS
  ${PCOMN_C_OPT_DBGINFO} ${PCOMN_C_OPT_CHECK} ${PCOMN_C_OPT_TRACE}
  )

force_flags(CMAKE_C_FLAGS ${PCOMN_C_OPT_BASE})

force_flags(CMAKE_CXX_FLAGS
  ${PCOMN_CXX_OPT_CXX11} # Use C++11 for build
  ${PCOMN_CXX_OPT_BASE}
  )

add_library(pcommon STATIC

  pcomn_aprintf.c
  regex.c

  pcomn_crc32.c
  pcomn_hash.c
  pcomn_version.c
  pudate.c

  pcomn_trace.cpp
  pcomn_tracecfg.cpp

  pcomn_binascii.cpp
  pcomn_cfgparser.cpp
  pcomn_crypthash.cpp
  pcomn_diag.cpp
  pcomn_fileutils.cpp
  pcomn_incdlist.cpp
  pcomn_iostream.cpp

  pcomn_journal.cpp
  pcomn_journstorage.cpp
  pcomn_journfile.cpp
  pcomn_journmmap.cpp

  pcomn_mmap.cpp
  pcomn_path.cpp
  pcomn_qualname.cpp
  pcomn_rawstream.cpp
  pcomn_regex.cpp
  pcomn_scheduler.cpp
  pcomn_strsubst.cpp
  pcomn_textio.cpp
  pcomn_thread.cpp
  pcomn_threadpool.cpp
  pcomn_uri.cpp

  pcomn_ziowrap.c
  pcomn_zstream.cpp

  # Platform sources
  ${PLATFORM_SOURCES}/pcomn_nrthreadmutex.c
  ${PLATFORM_SOURCES}/pcomn_exec.cpp
  ${PLATFORM_SOURCES}/pcomn_shutil.cpp
  ${PLATFORM_SOURCES}/pcomn_sys.cpp
)

target_include_directories(pcommon
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  )

target_compile_options(pcommon
  PUBLIC $<$<CONFIG:Debug>:${PCOMN_DEBUG_FLAGS}>
  )
