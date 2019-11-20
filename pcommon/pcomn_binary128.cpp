/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_binary128.cpp
 COPYRIGHT    :   Yakov Markovitch, 2010-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Large fixed binary types (128- and 256-bit)
 CREATION DATE:   30 Jul 2010
*******************************************************************************/
#include "pcomn_binary128.h"
#include "pcomn_binascii.h"

#include <iostream>

namespace pcomn {

/*******************************************************************************
 b128_t
*******************************************************************************/
std::string b128_t::to_string() const { return b2a_hex(data(), size()) ; }

char *b128_t::to_strbuf(char *buf) const
{
   b2a_hex(data(), size(), buf) ;
   buf[slen()] = 0 ;
   return buf ;
}

/*******************************************************************************
 binary256_t
*******************************************************************************/
std::string binary256_t::to_string() const
{
   char buf[binary256_t::slen() + 1] ;
   return to_strbuf(buf) ;
}

/*******************************************************************************
 ostream
*******************************************************************************/
std::ostream &operator<<(std::ostream &os, const b128_t &v)
{
   char buf[b128_t::slen() + 1] ;
   return os.write(v.to_strbuf(buf), b128_t::slen()) ;
}

std::ostream &operator<<(std::ostream &os, const binary256_t &v)
{
   char buf[binary256_t::slen() + 1] ;
   return os.write(v.to_strbuf(buf), binary256_t::slen()) ;
}

} // namespace pcomn
