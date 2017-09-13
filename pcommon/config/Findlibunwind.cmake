# - Try to find libunwind
#
#  LIBUNWIND_FOUND - system has libunwind
#  LIBUNWIND_INCLUDE_DIR - the libunwind include directory
#  LIBUNWIND_LIBRARIES - Link these to use libunwind
#  LIBUNWIND_DEFINITIONS - Compiler switches required for using libunwind

find_path(LIBUNWIND_INCLUDE_DIR libunwind.h
  /usr/include
  /usr/local/include
)

find_library(LIBUNWIND_LIBRARIES NAMES unwind)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libunwind "Could not find libunwind" LIBUNWIND_INCLUDE_DIR LIBUNWIND_LIBRARIES)
# show the LIBUNWIND_INCLUDE_DIR and LIBUNWIND_LIBRARIES variables only in the advanced view
mark_as_advanced(LIBUNWIND_INCLUDE_DIR LIBUNWIND_LIBRARIES)
