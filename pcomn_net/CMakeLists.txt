#------------------------------------------------------------------------------
# FILE         :  CMakeLists.txt
# COPYRIGHT    :  Yakov Markovitch, 2014-2019
#                 See LICENSE for information on usage/redistribution.
#
# DESCRIPTION  :  netreach library build script
#
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  8 Sep 2014
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.12)

add_library(pcomn_net STATIC netsockets.cpp)

target_link_libraries(pcomn_net PUBLIC pcommon)
target_include_directories(pcomn_net PUBLIC ..)
