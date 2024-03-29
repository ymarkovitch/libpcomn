#------------------------------------------------------------------------------
# FILE         :  CMakeLists.txt
# COPYRIGHT    :  Yakov Markovitch, 2014-2020
#                 See LICENSE for information on usage/redistribution.
#
# DESCRIPTION  :  pcommon unittests build script
#
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  11 Sep 2014
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.12)

project(pcommn_unittests)

unittests_project(libpcommon_unittests)

set_project_link_libraries(pcommon)

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
unittest(unittest_unique_value)

unittest(unittest_algorithms)
unittest(unittest_base64)
unittest(unittest_binary128)

unittest_NOSIMD(unittest_bitops)
unittest_SSE42(unittest_bitops_SSE42)
unittest_AVX  (unittest_bitops_AVX)
unittest_AVX2 (unittest_bitops_AVX2)

unittest_SSE42(unittest_bitarray)
unittest_AVX  (unittest_bitarray_AVX)
unittest_AVX2 (unittest_bitarray_AVX2)

unittest_SSE42(unittest_bitvector)
unittest_AVX  (unittest_bitvector_AVX)
unittest_AVX2 (unittest_bitvector_AVX2)

unittest(unittest_blocqueue)

unittest(unittest_buffer)
unittest(unittest_cacher)
unittest(unittest_crypthash)
unittest(unittest_ctype)
unittest(unittest_enum)
unittest(unittest_exec)
unittest(unittest_file_lock)
unittest(unittest_filesys)
unittest(unittest_fixringbuf)
unittest(unittest_hash)
unittest(unittest_hashclosed)
unittest(unittest_ident_dispenser)
unittest(unittest_inclist)
unittest(unittest_iostream)
unittest(unittest_iterator)
unittest(unittest_memringbuf)
unittest(unittest_mmap)
unittest(unittest_network_address)
unittest(unittest_omanip)
unittest(unittest_rawstream)
unittest(unittest_regex)
unittest(unittest_semaphore)
unittest(unittest_simplematrix)
unittest(unittest_strnum)
unittest(unittest_strsubst)
unittest(unittest_sync)
unittest(unittest_sys)
unittest(unittest_socket_address)
unittest(unittest_ternary)
unittest(unittest_textio)

unittest(unittest_threadpool)
unittest(fuzzytest_threadpool)

unittest(unittest_timespec)
unittest(unittest_uri)
unittest(unittest_uuid)
unittest(unittest_vector)
unittest(unittest_zstream)

if (ZSTD_FOUND)
    unittest(unittest_zdict)
endif()

add_adhoc_executable(test_readfile)
add_adhoc_executable(test_backtrace)
add_adhoc_executable(test_stacktrace)
add_adhoc_executable(test_common_type)
add_adhoc_executable(test_rm)

add_adhoc_executable(benchmark_mmap)
add_adhoc_executable(benchmark_bin128hash)
add_adhoc_executable(sptr)
