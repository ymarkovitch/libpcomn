#------------------------------------------------------------------------------
# FILE         :  CMakeLists.txt
#
# DESCRIPTION  :  pcommon unittests build script
#
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  11 Sep 2014
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.12)

project(pcommon.unittests C CXX)

set_project_link_libraries(crypto z)

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
unittest(unittest_keyedset)
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
unittest(unittest_uuid)
unittest(unittest_zstream)

# pcomn::journal
unittest(unittest_journal_files)
unittest(unittest_journal)
