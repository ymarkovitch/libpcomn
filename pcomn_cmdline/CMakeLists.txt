#------------------------------------------------------------------------------
# FILE         :  CMakeLists.txt
# COPYRIGHT    :  Yakov Markovitch, 2014-2016
#                 See LICENSE for information on usage/redistribution.
#
# DESCRIPTION  :  cmdline library build script
#
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  8 Sep 2014
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5.0)

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

target_link_libraries(pcomn_cmdline PUBLIC pcommon)
target_include_directories(pcomn_cmdline PUBLIC ..)
