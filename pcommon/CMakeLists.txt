#------------------------------------------------------------------------------
# FILE         :  CMakeLists.txt
#
# COPYRIGHT    :  Yakov Markovitch, 2014-2018
#                 See LICENSE for information on usage/redistribution.
#
# DESCRIPTION  :  CMake script for building pcommon library
#
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  29 Aug 2014
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.12)

project(libpcommon C CXX)

# We have some custom .cmake scripts in pcommon/config
list(APPEND CMAKE_MODULE_PATH "${libpcommon_SOURCE_DIR}/config")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} CACHE PATH "")
set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS TRUE)

include(pcommon)

set(ENABLE_PCOMN_UNITTESTS  ON  CACHE BOOL "Enable unittests building. Requires CPPUntit.")
set(ENABLE_PCOMN_STACKTRACE OFF CACHE BOOL "Enable stacktrace classes. Linux-only. Requires libunwind and libdw.")
set(ENABLE_PCOMN_AVX        NO  CACHE BOOL "Enable use of AVX instruction set. Substantially accelerates e.g. hashing and pcomn::bitarray.")
set(ENABLE_PCOMN_AVX2       NO  CACHE BOOL "Enable use of AVX2 instruction set. Substantially accelerates e.g. hashing and pcomn::bitarray.")

message("***** Processing ${PROJECT_NAME} *****")

if (ENABLE_PCOMN_STACKTRACE AND (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    message(FATAL_ERROR "ENABLE_PCOMN_STACKTRACE=ON is supported only for Linux platform.")
endif()

if (${CMAKE_GENERATOR} MATCHES ".*Visual Studio.*")
    if (NOT "${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64")
        message(FATAL_ERROR "32-bit compilation is not supported. Please specify -DCMAKE_GENERATOR_PLATFORM=x64")
    endif()
    set(COMPILE_LANGUAGE_CXX "1:1")
else()
    set(COMPILE_LANGUAGE_CXX "COMPILE_LANGUAGE:CXX")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(COMPILER_GCCLIKE 1)
else()
    set(COMPILER_GCCLIKE 0)
endif()

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

function(find_required_package package_name package_checkvar)
    find_package(${package_name} REQUIRED)
    if (NOT ${package_checkvar})
        message(FATAL_ERROR
            "\n${package_name} library, which is required for stack trace, is not found.\n"
            "You probably need to install ${package_name}-devel or ${package_name}-dev package.\n")
    endif()
endfunction()

if (ENABLE_PCOMN_STACKTRACE)
    find_required_package(libelf    LIBELF_FOUND)
    find_required_package(libunwind LIBUNWIND_FOUND)
    find_required_package(libdw     LIBDW_FOUND)
endif()

string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE)

if (CMAKE_BUILD_TYPE MATCHES "REL")
    set(CMAKE_OPTIMIZED_BUILD TRUE)
else()
    set(CMAKE_OPTIMIZED_BUILD FALSE)
endif()

if (NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(ENABLE_PCOMN_AVX2 NO)
    set(ENABLE_PCOMN_AVX  NO)
endif()

if (ENABLE_PCOMN_AVX2 OR ENABLE_PCOMN_AVX)
    set(DISABLE_PCOMN_AVX NO)
    if (ENABLE_PCOMN_AVX2)
        set(ENABLE_PCOMN_AVX NO)
    endif()
else()
    set(DISABLE_PCOMN_AVX YES)
endif()

################################################################################
# Platform sources subdirectory
################################################################################
if (UNIX)
  set(PLATFORM_SOURCES unix)
else()
  set(PLATFORM_SOURCES win32)
endif(UNIX)

set(T1HA_SOURCEDIR t1ha/src)
set(T1HA_SOURCEFILES

    ${T1HA_SOURCEDIR}/t1ha0.c
    ${T1HA_SOURCEDIR}/t1ha1.c
    ${T1HA_SOURCEDIR}/t1ha2.c
    ${T1HA_SOURCEDIR}/t1ha0_ia32aes_avx.c
    ${T1HA_SOURCEDIR}/t1ha0_ia32aes_avx2.c
    ${T1HA_SOURCEDIR}/t1ha0_ia32aes_noavx.c
    )

add_library(pcommon STATIC

  regex.c

  pcomn_crc32.c

  ${T1HA_SOURCEFILES}

  pcomn_trace.cpp
  pcomn_tracecfg.cpp

  pcomn_binascii.cpp
  pcomn_binstream.cpp
  pcomn_cfgparser.cpp
  pcomn_diag.cpp
  pcomn_exec.cpp
  pcomn_fileutils.cpp
  pcomn_fileread.cpp

  pcomn_immutablestr.cpp
  pcomn_mmap.cpp
  pcomn_netaddr.cpp
  pcomn_path.cpp
  pcomn_rawstream.cpp
  pcomn_regex.cpp
  pcomn_ssafe.cpp
  pcomn_strsubst.cpp
  pcomn_textio.cpp
  pcomn_threadpool.cpp
  pcomn_uri.cpp
  pcomn_uuid.cpp

  pcomn_ziowrap.c
  pcomn_zstream.cpp

  $<$<NOT:$<PLATFORM_ID:Windows>>:
  pcomn_crypthash
  unix/pcomn_posix_exec.cpp
  unix/pcomn_shutil.cpp
  unix/pcomn_sys.cpp
  $<$<BOOL:${ENABLE_PCOMN_STACKTRACE}>:unix/pcomn_stacktrace.cpp>
  >
)

# C++14 required
target_compile_features(pcommon
  PUBLIC
  cxx_alignas
  cxx_alignof
  cxx_constexpr
  cxx_decltype
  cxx_delegating_constructors
  cxx_return_type_deduction
  cxx_relaxed_constexpr
  cxx_lambda_init_captures
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
  $<$<AND:${COMPILER_GCCLIKE},$<BOOL:${DISABLE_PCOMN_AVX}>>:-march=westmere>
  $<$<AND:${COMPILER_GCCLIKE},$<BOOL:${ENABLE_PCOMN_AVX}>>:-march=corei7-avx>
  $<$<AND:${COMPILER_GCCLIKE},$<BOOL:${ENABLE_PCOMN_AVX2}>>:-march=haswell>

  PRIVATE
  $<$<CXX_COMPILER_ID:GNU>:-Wall -Wno-error -Wno-unused-result>
  $<$<CXX_COMPILER_ID:Clang>:-Wall -Wno-error -Wno-unused-result -Wno-mismatched-tags>
  $<$<AND:${COMPILER_GCCLIKE},$<${COMPILE_LANGUAGE_CXX}>>:-Woverloaded-virtual>

  $<$<AND:$<OR:$<CONFIG:Debug>,$<CONFIG:DbgSanitize>,$<CONFIG:RelWithDebInfo>>,${COMPILER_GCCLIKE}>:-gdwarf-4>
  $<$<PLATFORM_ID:Linux>:-fvisibility=hidden>

  PUBLIC
  $<$<AND:$<CXX_COMPILER_ID:Clang>,$<${COMPILE_LANGUAGE_CXX}>>:
        -Wno-potentially-evaluated-expression -Wno-mismatched-tags
        -Wno-null-dereference -Wno-gnu>
  )

target_link_libraries(pcommon
  PUBLIC
  $<$<OR:$<PLATFORM_ID:Linux>,$<PLATFORM_ID:Cygwin>,$<PLATFORM_ID:MinGW>>:-pthread>
  $<$<BOOL:${ENABLE_PCOMN_STACKTRACE}>:gcc_s>
  $<$<BOOL:${ENABLE_PCOMN_STACKTRACE}>:unwind>
  $<$<BOOL:${ENABLE_PCOMN_STACKTRACE}>:dw>
  $<$<BOOL:${ENABLE_PCOMN_STACKTRACE}>:elf>
  )

set_source_files_properties(${T1HA_SOURCEFILES} PROPERTIES
    COMPILE_OPTIONS $<$<BOOL:${CMAKE_OPTIMIZED_BUILD}>:-O3>)

set_source_files_properties(${T1HA_SOURCEDIR}/t1ha0_ia32aes_noavx.c PROPERTIES COMPILE_OPTIONS -maes)
set_source_files_properties(${T1HA_SOURCEDIR}/t1ha0_ia32aes_avx.c   PROPERTIES COMPILE_OPTIONS "-maes;-mavx")
set_source_files_properties(${T1HA_SOURCEDIR}/t1ha0_ia32aes_avx2.c  PROPERTIES COMPILE_OPTIONS "-maes;-mavx;-mavx2")

if (ENABLE_PCOMN_UNITTESTS)
  enable_testing()
  add_subdirectory(unittests EXCLUDE_FROM_ALL)
endif()
