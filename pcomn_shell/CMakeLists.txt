#------------------------------------------------------------------------------
# FILE         :  CMakeLists.txt
# COPYRIGHT    :  Yakov Markovitch, 2017
#                 See LICENSE for information on usage/redistribution.
#
# DESCRIPTION  :  pcomn_cmdline library build script
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  29 Nov 2017
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.7)

project(libpcomn_shell)

add_library(pcomn_shell STATIC
  shell_cmdline.cpp
  shell_repl.cpp
  )

target_include_directories(pcomn_shell PUBLIC .. ../pcommon)

target_link_libraries(pcomn_shell PUBLIC pcomn_cmdline readline)
