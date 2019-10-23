#------------------------------------------------------------------------------
# FILE         :  CMakeLists.txt
# COPYRIGHT    :  Yakov Markovitch, 2019
#                 See LICENSE for information on usage/redistribution.
#
# DESCRIPTION  :
#
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  21 Oct 2019
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.12.0)

project(libpcomn_trie)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../pcommon/config")
include(pcommon)

add_library(pcomn_trie STATIC pcomn_netprefix.cpp)

target_link_libraries(pcomn_trie PUBLIC pcommon)
target_include_directories(pcomn_trie INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

enable_testing()
add_subdirectory(unittests EXCLUDE_FROM_ALL)
