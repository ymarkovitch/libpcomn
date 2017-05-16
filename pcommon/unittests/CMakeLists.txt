#------------------------------------------------------------------------------
# FILE         :  CMakeLists.txt
# COPYRIGHT    :  Yakov Markovitch, 2014-2016
#                 See LICENSE for information on usage/redistribution.
#
# DESCRIPTION  :  pcommon unittests build script
#
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  11 Sep 2014
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5.2)

unittests_project(pcommon)

set_project_link_libraries(crypto z)

# Test cfgparser first, to ensure diagnostics log configuration is functioning properly
unittest(unittest_cfgparser)

# Test test the most basic functionality used throughout the library
unittest(unittest_metafunction)
unittest(unittest_utils)

# Test the library
#
unittest(unittest_immutablestr)
#unittest(unittest_smartptr)
unittest(unittest_strshims)
unittest(unittest_strslice)

unittest(unittest_cacher)
unittest(unittest_cdsqueue)
unittest(unittest_cdscrq)
unittest(unittest_crypthash)
unittest(unittest_enum)
unittest(unittest_exec)
unittest(unittest_file_lock)
unittest(unittest_filesys)
unittest(unittest_hashclosed)
unittest(unittest_inclist)
unittest(unittest_integer)
unittest(unittest_iostream)
unittest(unittest_iterator)
unittest(unittest_mmap)
unittest(unittest_omanip)
unittest(unittest_rawstream)
unittest(unittest_regex)
unittest(unittest_simplematrix)
unittest(unittest_strnum)
unittest(unittest_strsubst)
unittest(unittest_sync)
unittest(unittest_synccomplex)
unittest(unittest_sys)
unittest(unittest_textio)
unittest(unittest_timespec)
unittest(unittest_uri)
unittest(unittest_uuid)
unittest(unittest_zstream)

add_adhoc_executable(test_readfile)
