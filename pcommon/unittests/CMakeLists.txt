#------------------------------------------------------------------------------
# FILE         :  CMakeLists.txt
#
# DESCRIPTION  :  pcommon unittests build script
#
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  11 Sep 2014
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.12)

set(PROJLIBS
  ../pcommon
  )

add_executable(unittest_integer unittest_integer.cpp)
target_link_libraries(unittest_integer ${PROJLIBS})
set_target_properties(unittest_integer PROPERTIES EXCLUDE_FROM_ALL true)

add_test(unittest_integer ${CMAKE_CTEST_COMMAND}
  --build-and-test "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}"
  --build-nocmake --build-noclean
  --build-generator "${CMAKE_GENERATOR}"
  --build-target unittest_integer
  --build-makeprogram "${CMAKE_MAKE_PROGRAM}"
  --test-command ${CMAKE_CURRENT_BINARY_DIR}/unittest_integer
  )
