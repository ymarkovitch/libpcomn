/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_except.cpp
 COPYRIGHT    :   Yakov Markovitch, 2000-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Base PCOMMON exception classes

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Feb 2000
*******************************************************************************/
#include <pcomn_except.h>
#include <win32/pcomn_w32util.h>

namespace pcomn {

int system_error::errcode::lasterr()
{
   return GetLastError() ;
}

std::string system_error::errcode::errmsg(int code)
{
   return sys_error_text(code) ;
}

} ;
