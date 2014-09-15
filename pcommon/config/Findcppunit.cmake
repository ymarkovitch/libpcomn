# Find cppunit includes and library
#
# This module defines
#  cppunit_INCLUDE_DIRS
#  cppunit_LIBRARIES, the libraries to link against to use cppunit.
#  cppunit_LIBRARY_DIRS, the location of the libraries
#  cppunit_FOUND, If false, do not try to use cppunit
#
# Copyright © 2007, Matt Williams
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

macro(Findcppunit_local_find)

  if (WIN32) #Windows
    set(cppunit_INCLUDE_SEARCH_DIRS
      ${cppunit_LIBRARY_SEARCH_DIRS}
      ${CMAKE_INCLUDE_PATH}
      /usr/include/
      /usr/local/
      /opt/include/
      )

    set(cppunit_LIBRARY_SEARCH_DIRS
      ${cppunit_LIBRARY_SEARCH_DIRS}
      ${CMAKE_LIBRARY_PATH}
      /usr/lib
      /usr/local/lib
      /opt/lib
      )
    find_path(cppunit_INCLUDE_DIRS cppunit/Test.h ${cppunit_INCLUDE_SEARCH_DIRS})
    find_library(cppunit_LIBRARIES cppunit PATHS ${cppunit_LIBRARY_SEARCH_DIRS})
    set_global(cppunit_INCLUDEDIR ${cppunit_INCLUDE_DIRS})
  else() #Unix
    cmake_minimum_required(VERSION 2.8.12)
    find_package(PkgConfig)
    pkg_search_module(cppunit cppunit)
    set_global(cppunit_INCLUDEDIR ${cppunit_INCLUDEDIR})
  endif (WIN32)

  #Do some preparation
  separate_arguments(cppunit_INCLUDE_DIRS)
  separate_arguments(cppunit_LIBRARIES)

  set(cppunit_INCLUDE_DIRS ${cppunit_INCLUDE_DIRS} CACHE STRING "")
  set(cppunit_LIBRARY_DIRS ${cppunit_LIBDIR} CACHE STRING "")
  set(cppunit_LIBRARIES ${cppunit_LIBRARIES} CACHE STRING "")

  mark_as_advanced(cppunit_INCLUDE_DIRS cppunit_LIBRARIES cppunit_LIBRARY_DIRS)

  message(STATUS "  libraries : ${cppunit_LIBRARIES} from ${cppunit_LIBDIR}")
  message(STATUS "  include   : ${cppunit_INCLUDEDIR}")

endmacro(Findcppunit_local_find)

if (cppunit_INCLUDEDIR AND cppunit_LIBRARIES)
  set(cppunit_FOUND TRUE)
else()
  Findcppunit_local_find()
endif()

if (NOT cppunit_FOUND AND cppunit_FIND_REQUIRED)
  message(FATAL_ERROR "Could not find cppunit")
endif()
