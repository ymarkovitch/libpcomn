#------------------------------------------------------------------------------
# FILE         :  CMakeLists.txt
#
# COPYRIGHT    :  Yakov Markovitch, 2014
#                 See LICENSE for information on usage/redistribution.
#
# DESCRIPTION  :  netreach library build script
#
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  8 Sep 2014
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.12)

add_library(pcomn_net STATIC
  netaddr.cpp
  netsockets.cpp
  )

target_link_libraries(pcomn_net PUBLIC pcommon)

force_flags(CMAKE_CXX_FLAGS
  ${PCOMN_CXX_OPT_CXX11} # Use C++11 for build
  ${PCOMN_CXX_OPT_BASE}
  )

target_include_directories(pcomn_net
  PUBLIC
  ${PROJSHARE}
  ${PROJPCOMN}
  )
