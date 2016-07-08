#------------------------------------------------------------------------------
# FILE         :  pcommon.cmake
# COPYRIGHT    :  Yakov Markovitch, 2014-2016
#                 See LICENSE for information on usage/redistribution.
#
# DESCRIPTION  :  pcommon CMake module implementing various basic CMake utilities
#
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  5 Sep 2014
#------------------------------------------------------------------------------
if(__PCOMMON_INCLUDED)
  return()
endif()
set(__PCOMMON_INCLUDED CACHE INTERNAL "")

################################################################################
# set_global(var)
# Force setting CACHE INTERNAL ("global") variable ${var}
# If no value specified, delete ${var} from the cache
################################################################################
macro(set_global var)
  if (${ARGC})
    set(${var} ${ARGN} CACHE INTERNAL "")
  else()
    unset(${var} CACHE)
  endif()
endmacro(set_global)

################################################################################
# A family for set manipulation macros
# set_diff
# set_union
################################################################################
macro(set_diff inout_set1 in_set2)
  foreach(dummy IN LISTS ${in_set2})
    list(REMOVE_ITEM ${inout_set1} ${${in_set2}})
    break()
  endforeach()
endmacro(set_diff)

macro(set_union inout_set1 in_set2)
  # Hack: create a local variable in a macro scope
  foreach(dummy IN LISTS ${in_set2})
    set(set2 ${${in_set2}})
    set_diff(set2 ${inout_set1})
    list(APPEND ${inout_set1} ${set2})
    break()
  endforeach()
endmacro(set_union)

################################################################################
# Join elements of list ${var} into a string using ${delim} delimiter and assign
# the result to ${var}
# Does not attempt to remove duplicate arguments already existing in ${var}.
################################################################################
macro(list_join var delim)
  foreach(result "")
    set(_delim "")
    foreach(arg IN LISTS ${var})
      set(result "${result}${_delim}${arg}")
      set(_delim "${delim}")
    endforeach(arg)
    set(${var} ${result})
  endforeach(result)
endmacro(list_join)

################################################################################
# Add arguments not found in ${var} to the end of ${var}
# Does not attempt to remove duplicate arguments already existing in ${var}.
################################################################################
macro(force_flags var)
  foreach(arglist ${ARGN})
    set(arglist ${${var}})
    # Create a separated list of the arguments
    separate_arguments(arglist)
    foreach(opts RANGE 0)
      set(opts ${ARGN})
      set_union(arglist opts)
    endforeach()
    set(${var} ${arglist})
    list_join(${var} " ")
    break()
  endforeach()
endmacro(force_flags)

################################################################################
# Print all defined variables (debugging macro)
################################################################################
macro(print_all_variables)
  get_cmake_property(vars_ VARIABLES)
  foreach (var_ ${vars_})
    message(STATUS "${var_}=${${var_}}")
  endforeach()
endmacro(print_all_variables)

################################################################################
#
# Global variables
#
################################################################################

set_global(PCOMN_CONFIG ${CMAKE_CURRENT_LIST_DIR})

################################################################################
# Portable C/C++ compiler options
################################################################################
if (UNIX)
  set_global(PCOMN_C_OPT_BASE    -march=corei7 -pthread -fvisibility=hidden -Wall)
  set_global(PCOMN_C_OPT_DBGINFO -gdwarf-4 -fno-debug-types-section -grecord-gcc-switches)

  set_global(PCOMN_CXX_OPT_BASE  ${PCOMN_C_OPT_BASE} -Woverloaded-virtual)
else()
  set_global(PCOMN_C_OPT_BASE "")
  set_global(PCOMN_C_OPT_DBGINFO "")

  set_global(PCOMN_CXX_OPT_BASE "")
endif(UNIX)

set_global(PCOMN_TRACE __PCOMN_TRACE)
set_global(PCOMN_CHECK __PCOMN_DEBUG=2)

################################################################################
# Portable COMMAND arguments for using in add_custom_command and execute_process
################################################################################
# cmake
set_global(CMD_CMAKE ${CMAKE_COMMAND})
# mv from to
set_global(CMD_MV    ${CMAKE_COMMAND} -E rename)
# cp from to
set_global(CMD_CP    ${CMAKE_COMMAND} -E copy)
# rm file1 [file2 ...]
set_global(CMD_RM    ${CMAKE_COMMAND} -E remove)
# echo
set_global(CMD_ECHO  ${CMAKE_COMMAND} -E echo)
# echo -n (no endline)
set_global(CMD_ECHON ${CMAKE_COMMAND} -E echo_append)
# cat file
if (WIN32)
  set_global(CMD_CAT cmd /c type)
else()
  set_global(CMD_CAT cat)
endif(WIN32)

################################################################################
# Unittest handling
################################################################################
function(unittests_directory)

  get_directory_property(is_unittests_directory DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} UNITTESTS_DIRECTORY)
  if (is_unittests_directory)
    return()
  endif()

  add_custom_target(unittests.start COMMENT
    "\n+++++++++++++++++++++++ Building and running unittests +++++++++++++++++++++++")
  add_custom_target(unittests DEPENDS unittests.start)
  add_custom_command(TARGET unittests POST_BUILD COMMENT
    "+++++++++++++++++++++++ Finished running unittests +++++++++++++++++++++++++++")

  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY UNITTESTS_DIRECTORY TRUE)

endfunction(unittests_directory)

macro(set_executable_path path)
  if(IS_ABSOLUTE ${path})
    set(EXECUTABLE_OUTPUT_PATH ${path} PARENT_SCOPE)
  else()
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin PARENT_SCOPE)
  endif()
  set(LIBRARY_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH} PARENT_SCOPE)
endmacro(set_executable_path)

################################################################################
# unittest(<name> [source1 ...])
#
# Add an executable unittest target called <name> to be built either from
# <name>.cpp if there are no source files specified or from the source files
# listed in the command invocation.
################################################################################
function(unittest name)

  set_executable_path(bin)

  find_package(cppunit REQUIRED)

  if (" ${ARGN}" STREQUAL " ")
    set(sources ${name}.cpp)
  else()
    set(sources ${ARGN})
  endif()

  if (NOT WIN32)
    set(LAST_EXIT_STATUS "$$?")
  else()
    set(LAST_EXIT_STATUS "%ERRORLEVEL%")
  endif()

  # Unittest executable
  add_executable(${name} ${sources})

  # All unittests depend on pcommon and cppunit libraries
  target_link_libraries(${name} PRIVATE pcommon cppunit)
  apply_project_requirements(${name})

  set_target_properties(${name} PROPERTIES EXCLUDE_FROM_ALL true)

  add_custom_command(
    OUTPUT  ${EXECUTABLE_OUTPUT_PATH}/${name}.run
    WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
    DEPENDS ${name}

    COMMAND ${CMD_RM} ${name}.output ${name}.run
    COMMAND ${CMD_ECHO} "  running: ${name}"

    COMMAND
    PCOMN_TESTDIR=${CMAKE_CURRENT_SOURCE_DIR} flock -w 7200 ${CMAKE_BINARY_DIR} $<TARGET_FILE:${name}> >${name}.output 2>&1 ||
    (${CMD_CMAKE} -DUNITTEST_OUTPUT=${name}.output -DUNITTEST_STATUS=${LAST_EXIT_STATUS} -P ${PCOMN_CONFIG}/put_status.cmake && exit 1)

    COMMAND ${CMD_CMAKE} -DUNITTEST_OUTPUT=${name}.output -DUNITTEST_STATUS=0 -P ${PCOMN_CONFIG}/put_status.cmake
    COMMAND ${CMD_ECHO} "**passed** ${name}"
    COMMAND ln ${name}.output ${name}.run
    )

  add_custom_target(${name}.test DEPENDS ${EXECUTABLE_OUTPUT_PATH}/${name}.run)
  add_test(NAME ${name} WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${name})

  add_dependencies(unittests ${name}.test)

endfunction(unittest)

function(set_project_link_libraries)
  if (ARGC GREATER 0)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY PCOMN_PROJECT_LINK_LIBRARIES ${ARGN})
  endif()
endfunction(set_project_link_libraries)

function(apply_project_requirements target1)
  set(targets ${target1} ${ARGN})

  get_property(link_requirements DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY PCOMN_PROJECT_LINK_LIBRARIES)

  foreach(target IN LISTS targets)
    target_link_libraries(${target} PRIVATE ${link_requirements})
  endforeach()
endfunction()

unittests_directory()
add_custom_target(check DEPENDS unittests)
