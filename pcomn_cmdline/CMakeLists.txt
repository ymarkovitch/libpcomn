#------------------------------------------------------------------------------
# FILE         :  CMakeLists.txt
# COPYRIGHT    :  Yakov Markovitch, 2017
#                 See LICENSE for information on usage/redistribution.
#
# DESCRIPTION  :  pcomn_cmdline library build script
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  29 Nov 2017
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5.2)

project(libpcomn_cmdline)

add_library(pcomn_cmdline STATIC
  argiter.cpp
  cmdarg.cpp
  cmdargs.cpp
  cmdext.cpp
  cmdline.cpp
  dump.cpp
  fifolist.cpp
  parse.cpp
  patchlevel.cpp
  private.cpp
  strindent.cpp
  unix.cpp
  usage.cpp
  )

target_include_directories(pcomn_cmdline PUBLIC .. ../pcommon)

add_executable(cmdtest cmdtest.cpp)
target_link_libraries(cmdtest pcomn_cmdline)

add_executable(cmdtestext cmdtestext.cpp)
target_link_libraries(cmdtestext pcomn_cmdline)

add_executable(cmdtestglob cmdtestglob.cpp)
target_link_libraries(cmdtestglob pcomn_cmdline)
