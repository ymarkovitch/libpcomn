#------------------------------------------------------------------------------
# FILE         :  pcommon.cmake
#
# COPYRIGHT    :  Yakov Markovitch, 2014
#
# DESCRIPTION  :  pcommon CMake module implementing various basic CMake utilities
#
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  5 Sep 2014
#------------------------------------------------------------------------------

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
    foreach(arg IN LISTS ${var})
      set(result "${result}${delim}${arg}")
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

function(set_compilation_features lang buildtype)
endfunction(set_compilation_features)

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
  set_global(PCOMN_C_OPT_BASE    -march=core2 -msse4.1 -pthread -fvisibility=hidden -Wall)
  set_global(PCOMN_C_OPT_DBGINFO -gdwarf-4 -fno-debug-types-section -grecord-gcc-switches)

  set_global(PCOMN_CXX_OPT_CXX11 -std=c++11)
  set_global(PCOMN_CXX_OPT_BASE  ${PCOMN_C_OPT_BASE} -Woverloaded-virtual)
else()
  set_global(PCOMN_C_OPT_BASE "")
  set_global(PCOMN_C_OPT_DBGINFO "")

  set_global(PCOMN_CXX_OPT_CXX11 "")
  set_global(PCOMN_CXX_OPT_BASE "")
endif(UNIX)

set_global(PCOMN_C_OPT_TRACE -D__PCOMN_TRACE)
set_global(PCOMN_C_OPT_CHECK -D__PCOMN_DEBUG=2)

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
