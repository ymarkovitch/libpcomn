# Try to find libdw, the elfutils' suite library for handling DWARF debugging format
#
#  LIBDW_FOUND       - system has libdw development package
#  LIBDW_INCLUDE_DIR - the libdw include directory
#  LIBDW_LIBRARIES   - Link these to use libdw
#  LIBDW_DEFINITIONS - Compiler switches required for using libdw

find_path(LIBDW_INCLUDE_DIR libdwfl.h
  /usr/include/elfutils
  /usr/local/include/elfutils
  /usr/include
  /usr/local/include
)

find_library(LIBDW_LIBRARIES NAMES dw)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libdw "Could not find libdw." LIBDW_INCLUDE_DIR LIBDW_LIBRARIES)
# Show the LIBDW_INCLUDE_DIR and LIBDW_LIBRARIES variables only in the advanced view
mark_as_advanced(LIBDW_INCLUDE_DIR LIBDW_LIBRARIES)
