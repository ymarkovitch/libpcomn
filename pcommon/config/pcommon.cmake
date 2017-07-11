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
get_property(__PCOMMON_INCLUDED GLOBAL PROPERTY __PCOMMON_INCLUDED SET)
if(__PCOMMON_INCLUDED)
  return()
endif()
set_property(GLOBAL PROPERTY __PCOMMON_INCLUDED TRUE)

# Do not implicitly dereference IF arguments corresponding to variables.
# Dereference variables only explicitly, like ${VAR}
cmake_policy(SET CMP0012 NEW)

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
# CMake does not allow linking OBJECT libraries for usage requirements
################################################################################
function(target_requirements target depends_on)
  target_include_directories(${target} PRIVATE $<TARGET_PROPERTY:${depends_on},INTERFACE_INCLUDE_DIRECTORIES>)
  target_compile_definitions(${target} PRIVATE $<TARGET_PROPERTY:${depends_on},INTERFACE_COMPILE_DEFINITIONS>)
  target_compile_features(${target} PRIVATE $<TARGET_PROPERTY:${depends_on},INTERFACE_COMPILE_FEATURES>)
  target_compile_options(${target} PRIVATE $<TARGET_PROPERTY:${depends_on},INTERFACE_COMPILE_OPTIONS>)
  add_dependencies(${target} ${depends_on})
endfunction(target_requirements)

################################################################################
#
# Global variables
#
################################################################################
set_global(PCOMN_CONFIG ${CMAKE_CURRENT_LIST_DIR})

################################################################################
# Portable C/C++ compiler options
################################################################################
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

function(unittests_project NAME)
    project(${NAME}.unittests CXX C)
    unittests_directory()
    add_custom_target(${NAME}.unittests DEPENDS unittests)
endfunction(unittests_project)

macro(set_executable_path path)
  if(IS_ABSOLUTE ${path})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${path})
  else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
  endif()
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} PARENT_SCOPE)
endmacro(set_executable_path)

################################################################################
# unittest(<name>
#          [SOURCES [source1...]]
#          [LIBS [lib1...]]
#          [DEFS [def1...]]
#          [OPTS [opt1...]])
#
# Add an executable unittest target called <name>.
# The target is built:
#   - from <name>.cpp if there are no [source1 ...] files specified or
#   - from the source files listed in the command invocation.
#
# unittest(foo) builds executable target foo from source file foo.cpp
#
# unittest(foo SOURCES bar.cpp quux.cpp) builds executable target foo from sources
# bar.cpp and quux.cpp
#
# Use set_project_link_libraries() to specify libraries and build requirements
# for _all_ the unittests in the project directory.
################################################################################
#
function(unittest name)

  cmake_parse_arguments(PARSE_ARGV 1
    PCOMN_UNITTEST
    "" # No options
    "" # No one_value_keywords
    "SOURCES;LIBS;OPTS;DEFS")

  if (NOT ("${PCOMN_UNITTEST_UNPARSED_ARGUMENTS}" STREQUAL ""))
    list_join(PCOMN_UNITTEST_UNPARSED_ARGUMENTS " ")
    message(FATAL_ERROR "Invalid arguments passed to unittest ${name}: ${PCOMN_UNITTEST_UNPARSED_ARGUMENTS}")
  endif()

  set_executable_path(bin)

  find_package(cppunit REQUIRED)

  if ("${PCOMN_UNITTEST_SOURCES}" STREQUAL "")
    set(sources ${name}.cpp)
  else()
    set(sources ${PCOMN_UNITTEST_SOURCES})
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

  target_link_libraries     (${name} PRIVATE ${PCOMN_UNITTEST_LIBS})
  target_compile_options    (${name} PRIVATE ${PCOMN_UNITTEST_OPTS})
  target_compile_definitions(${name} PRIVATE ${PCOMN_UNITTEST_DEFS} -DCPPUNIT_USE_TYPEINFO_NAME)

  set_target_properties(${name} PROPERTIES EXCLUDE_FROM_ALL true)

  get_target_property(BINARY_OUTPUT_DIR ${name} RUNTIME_OUTPUT_DIRECTORY)

  add_custom_command(
    OUTPUT  ${BINARY_OUTPUT_DIR}/${name}.run
    WORKING_DIRECTORY ${BINARY_OUTPUT_DIR}
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

    add_custom_target(${name}.test
        DEPENDS ${BINARY_OUTPUT_DIR}/${name}.run)

    add_custom_target(${name}.view
        DEPENDS ${name}.test
        COMMAND ${CMD_CAT} ${BINARY_OUTPUT_DIR}/${name}.run)

   add_test(NAME ${name} WORKING_DIRECTORY ${BINARY_OUTPUT_DIR} COMMAND ${name})

   add_dependencies(unittests ${name}.test)

endfunction(unittest)

################################################################################
# add_adhoc_executable(<name> [source1 ...])
#
# Add an executable target called <name>, built from <name>.cpp
#
# Use set_project_link_libraries() to specify libraries and build requirements
# for _all_ the ad-hoc executables in the project directory.
################################################################################
#
function(add_adhoc_executable name)

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

endfunction(add_adhoc_executable)

################################################################################
# set_project_link_libraries([library-target-1 ...])
#
# Specify libraries to use when linking all targets specified by unitest() or
# add_adhoc_executable() for the project directory the set_project_link_libraries()
# is called in.
#
# Since usage requirements from linked library targets are propagated and affect
# compilation of target sources, it is possible to set common build requirements
# through INTERFACE-only "libraries".
################################################################################
#
function(set_project_link_libraries)
  if (ARGC GREATER 0)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY PCOMN_PROJECT_LINK_LIBRARIES ${ARGN})
  endif()
endfunction(set_project_link_libraries)

# Apply requirements specified by set_project_link_libraries
function(apply_project_requirements target1)
  set(targets ${target1} ${ARGN})

  get_property(link_requirements DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY PCOMN_PROJECT_LINK_LIBRARIES)

  foreach(target IN LISTS targets)
    target_link_libraries(${target} PRIVATE ${link_requirements})
  endforeach()
endfunction()

#
# Prepare
#
unittests_directory()

if (TARGET check)
    add_dependencies(check unittests)
else()
    add_custom_target(check DEPENDS unittests)
endif()
