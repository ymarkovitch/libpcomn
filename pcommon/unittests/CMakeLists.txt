#------------------------------------------------------------------------------
# FILE         :  CMakeLists.txt
#
# DESCRIPTION  :  pcommon unittests build script
#
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  11 Sep 2014
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.12)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH})

add_custom_target(unittests.start COMMENT
  "\n+++++++++++++++++++++++ Building and running unittests +++++++++++++++++++++++")
add_custom_target(unittests DEPENDS unittests.start)
add_custom_command(TARGET unittests POST_BUILD COMMENT
  "+++++++++++++++++++++++ Finished running unittests +++++++++++++++++++++++++++")

add_custom_target(check DEPENDS unittests)

################################################################################
# unittest(<name> [source1 ...])
#
# Add an executable unittest target called <name> to be built either from
# <name>.cpp if there are no source files specified or from the source files
# listed in the command invocation.
################################################################################
function(unittest name)

  find_package(cppunit REQUIRED)

  if (" ${ARGN}" STREQUAL " ")
    set(sources ${name}.cpp)
  else()
    set(sources ${ARGN})
  endif()

  # Unittest executable
  add_executable(${name} ${sources})
  # All unittests depend on pcommon and cppunit libraries
  target_link_libraries(${name} PRIVATE pcommon cppunit)
  set_target_properties(${name} PROPERTIES EXCLUDE_FROM_ALL true)

  add_custom_command(
    OUTPUT  ${EXECUTABLE_OUTPUT_PATH}/${name}.run
    WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
    DEPENDS ${name}

    COMMAND ${CMD_RM} ${name}.output ${name}.run
    COMMAND ${CMD_ECHON} "  running: ${name} ..."
    COMMAND $<TARGET_FILE:${name}> >${name}.output 2>&1 || (${CMD_CMAKE} -DUNITTEST_OUTPUT=${name}.output -DUNITTEST_STATUS=${VAR_ERRORLEVEL} -P ../put_status.cmake && exit 1)
    COMMAND ${CMD_CMAKE} -DUNITTEST_OUTPUT=${name}.output -DUNITTEST_STATUS=0 -P ../put_status.cmake
    COMMAND ${CMD_ECHO} "**passed** ${name}"
    COMMAND ln ${name}.output ${name}.run
    )

  add_custom_target(${name}.test DEPENDS ${EXECUTABLE_OUTPUT_PATH}/${name}.run)
  add_test(NAME ${name} WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${name})

  add_dependencies(unittests ${name}.test)

endfunction(unittest)

# Test cfgparser first, to ensure diagnostics log configuration is functioning properly
unittest(unittest_cfgparser)

# Test test the most basic functionality used throughout the library
unittest(unittest_metafunction)
unittest(unittest_utils)

# Test the library
#
unittest(unittest_immutablestr)
unittest(unittest_smartptr)
unittest(unittest_strshims)
unittest(unittest_strslice)

unittest(unittest_cacher)
unittest(unittest_crypthash)
unittest(unittest_exec)
unittest(unittest_file_lock)
unittest(unittest_hashclosed)
unittest(unittest_inclist)
unittest(unittest_indicator_observer)
unittest(unittest_integer)
unittest(unittest_iostream)
#unittest(unittest_keyedmutex)
unittest(unittest_lanes)
unittest(unittest_mmap)
unittest(unittest_mxmutex)
unittest(unittest_rawstream)
unittest(unittest_regex)
unittest(unittest_safeformat)
unittest(unittest_simplematrix)
unittest(unittest_strnum)
unittest(unittest_strsubst)
unittest(unittest_sync)
unittest(unittest_synccomplex)
unittest(unittest_sys)
unittest(unittest_textio)
unittest(unittest_timespec)
unittest(unittest_uri)
unittest(unittest_zstream)

# pcomn::journal
unittest(unittest_journal_files)
unittest(unittest_journal)
