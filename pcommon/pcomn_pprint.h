/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_PPRINT_H
#define __PCOMN_PPRINT_H
/*******************************************************************************
 FILE         :   pcomn_pprint.h
 COPYRIGHT    :   Yakov Markovitch, 2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Pretty printing support

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   22 Dec 2015
*******************************************************************************/
/** @file
 Pretty printing support
*******************************************************************************/
#include <pcomn_utils.h>
#include <pcomn_omanip.h>

namespace pcomn {

template<typename C>
class pprint_basic_ostream ;

typedef pprint_basic_ostream<char> pprint_ostream ;

/******************************************************************************/
/** Implements tabulated output for any std::ostream
*******************************************************************************/
template<typename C>
class pprint_basic_ostream : private std::basic_streambuf<C>, public std::basic_ostream<C> {
      PCOMN_NONCOPYABLE(pprint_basic_ostream) ;
      PCOMN_NONASSIGNABLE(pprint_basic_ostream) ;

      typedef std::basic_ostream<C>    ancestor ;
      typedef std::basic_streambuf<C>  streambuf_type ;
   public:
      using typename ancestor::char_type ;
      using typename ancestor::int_type ;
      using typename ancestor::pos_type ;
      using typename ancestor::off_type ;
      using typename ancestor::traits_type ;

      explicit pprint_basic_ostream(std::basic_ostream<char_type> &actual_ostream) ;

      ~pprint_basic_ostream() { _actual_os.flush() ; }

      size_t line() { return refresh_linecount()._currrent_line ; }
      size_t column() { return this->pptr() - refresh_linecount()._startline_ptr ; }

      pprint_basic_ostream &skip_to_col(size_t column_num) ;

   private:
      std::basic_ostream<char_type> &  _actual_os ;
      size_t                           _currrent_line ;
      const char_type *                _linecount_ptr ;
      const char_type *                _startline_ptr ;
      std::unique_ptr<char_type[]>     _data ;

      enum : size_t { buf_capacity = 512 } ;

      pprint_basic_ostream &refresh_linecount() ;

   protected:
      void reset_buf()
      {
         this->setp(_data.get(), _data.get()) ;
         this->setg(_data.get(), _data.get(), _data.get()) ;
         _startline_ptr = _linecount_ptr = this->pbase() ;
      }

      int_type overflow(int_type c = traits_type::eof()) override ;
} ;

/*******************************************************************************
 pprint_basic_ostream
*******************************************************************************/
template<typename C>
pprint_basic_ostream<C>::pprint_basic_ostream(std::basic_ostream<char_type> &os) :
   _actual_os(os),
   _currrent_line(0),
   _linecount_ptr(nullptr),
   _startline_ptr(nullptr)
{}

template<typename C>
pprint_basic_ostream<C> &pprint_basic_ostream<C>::skip_to_col(size_t column_num)
{
   const size_t current_column = column() ;
   if (column_num < current_column)
      *this << std::setw(column_num - current_column) << "" ;
}

template<typename C>
pprint_basic_ostream<C> &pprint_basic_ostream<C>::refresh_linecount()
{
   while (_linecount_ptr != this->pptr())
      if (*_linecount_ptr++ == char_type('\n'))
      {
         ++_currrent_line ;
         _startline_ptr = _linecount_ptr ;
      }
   return *this ;
}

template<typename C>
typename pprint_basic_ostream<C>::int_type pprint_basic_ostream<C>::overflow(int_type c)
{
   if (!this->pptr())
   {
      NOXCHECK(!_data) ;
      if (!traits_type::eq_int_type(c, traits_type::eof()))
         _data.reset(new char_type[buf_capacity]) ;
   }
   else
   {
      refresh_linecount() ;
      _actual_os.write(this->pbase(), this->pptr() - this->pbase()) ;
   }
   reset_buf() ;
   if (traits_type::eq_int_type(c, traits_type::eof()))
   {
      _actual_os.flush() ;
      return traits_type::not_eof(c) ;
   }
   *this->pptr() = traits_type::to_char_type(c) ;
   this->pbump(1) ;
   return c ;
}

} // end of namespace pcomn

#endif /* __PCOMN_PPRINT_H */
