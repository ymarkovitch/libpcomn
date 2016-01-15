/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_hashclosed.compfail.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Compilation of this file should fail because pcomn::closed_hash_table
                  shouldn't allow non-POD values.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   20 Apr 2008
*******************************************************************************/
#include <pcomn_hashclosed.h>
#include <string>

void foo()
{
   pcomn::closed_hashtable<std::string> BadNonPodClosedHashtable ;
}
