#------------------------------------------------------------------------------
# FILE         :  CMakeLists.txt
# COPYRIGHT    :  Yakov Markovitch, 2019
#                 See LICENSE for information on usage/redistribution.
#
# DESCRIPTION  :
#
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  21 Oct 2019
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.12)

unittests_project(pcomn_trie_unittests)

set_project_link_libraries(pcommon pcomn_trie)

# Test the library
#
unittest(unittest_netprefix)
