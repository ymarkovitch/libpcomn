/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_immutablestr.cpp
 COPYRIGHT    :   Yakov Markovitch, 2019
 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   25 Jan 2019
*******************************************************************************/
#include "pcomn_immutablestr.h"
#include "pcomn_immutablestr.cc"

namespace pcomn {

/*******************************************************************************
 Force instantiation
*******************************************************************************/
template class immutable_string<char> ;
template class immutable_string<wchar_t> ;

template class mutable_strbuf<char> ;
template class mutable_strbuf<wchar_t> ;

} // end of namespace pcomn
