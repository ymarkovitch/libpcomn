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
  pcommon
  )

add_executable(unittest_integer unittest_integer.cpp)
target_link_libraries(unittest_integer PRIVATE ${PROJLIBS})
set_target_properties(unittest_integer PROPERTIES EXCLUDE_FROM_ALL true)

set_global(COMMAND_MV   ${CMAKE_COMMAND} -E rename)
set_global(COMMAND_RM   ${CMAKE_COMMAND} -E remove)
set_global(COMMAND_ECHO ${CMAKE_COMMAND} -E echo)
if (WIN32)
  set(VAR_ERRORLEVEL "%ERRORLEVEL%")
else()
  set(VAR_ERRORLEVEL "$?")
endif(WIN32)

add_custom_command(
  OUTPUT  bin/unittest_integer.run
  COMMAND ${COMMAND_RM} unittest_integer.output
  COMMAND $<TARGET_FILE:unittest_integer> >bin/unittest_integer.run 2>&1 || ${CMAKE_COMMAND} -P put_status.cmake unittest_integer.output ${VAR_ERRORLEVEL}
  COMMAND ${CMAKE_COMMAND} -P put_status.cmake unittest_integer.output 0
  COMMAND ${COMMAND_MV} unittest_integer.output unittest_integer.run
  )

add_custom_target(unittest_integer.test DEPENDS bin/unittest_integer.run)

#add_test(unittest_integer ${CMAKE_CTEST_COMMAND}
#  --build-and-test "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}"
#  --build-nocmake --build-noclean
#  --build-generator "${CMAKE_GENERATOR}"
#  --build-target unittest_integer
#  --build-makeprogram "${CMAKE_MAKE_PROGRAM}"
#  --test-command ${CMAKE_CURRENT_BINARY_DIR}/unittest_integer
#  )
