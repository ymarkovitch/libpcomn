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
if (UNIX)
  set(PCOMN_C_OPT_BASE    -march=core2 -msse4.1 -pthread -fvisibility=hidden -Wall CACHE INTERNAL "")
  set(PCOMN_C_OPT_DBGINFO -gdwarf-4 -fno-debug-types-section -grecord-gcc-switches CACHE INTERNAL "")

  set(PCOMN_CXX_OPT_CXX11 -std=c++11 CACHE INTERNAL "")
  set(PCOMN_CXX_OPT_BASE  ${PCOMN_C_OPT_BASE} -Woverloaded-virtual CACHE INTERNAL "")
else()
  set(PCOMN_C_OPT_BASE "" CACHE INTERNAL "")
  set(PCOMN_C_OPT_DBGINFO "" CACHE INTERNAL "")

  set(PCOMN_CXX_OPT_CXX11 "" CACHE INTERNAL "")
  set(PCOMN_CXX_OPT_BASE "" CACHE INTERNAL "")
endif(UNIX)

set(PCOMN_C_OPT_TRACE -D__PCOMN_TRACE CACHE INTERNAL "")
set(PCOMN_C_OPT_CHECK -D__PCOMN_DEBUG=2 CACHE INTERNAL "")

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

macro(list_join listvar delim)
  foreach(result "")
    foreach(arg IN LISTS ${listvar})
      set(result "${result}${delim}${arg}")
    endforeach(arg)
    set(${listvar} ${result})
  endforeach(result)
endmacro(list_join)

# This will add arguments not found in ${var} to the end of var.
# It does not attempt to remove duplicate arguments already existing
# in ${var}.
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
