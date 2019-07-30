/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_CSTRPTR_H
#define __PCOMN_CSTRPTR_H
/*******************************************************************************
 FILE         :   pcomn_cstrptr.h
 COPYRIGHT    :   Yakov Markovitch, 2010-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   C string (const char *) proxy object.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   7 Aug 2010
*******************************************************************************/
/** @file
 C string (const char *) proxy object.

 Useful as a function parameter object where serves as a "shim" converting
 to const char* any string type for which pcomn::string_traits<> is defined.
*******************************************************************************/
#include <pcomn_string.h>
#include <functional>

namespace pcomn {

/***************************************************************************//**
 C string (const char *) proxy object.
*******************************************************************************/
template<typename C, typename Deleter = void>
struct basic_cstrptr ;

/***************************************************************************//**
 Implementation of basic_cstrptr
*******************************************************************************/
template<typename C>
struct basic_cstrptr_base {

      typedef C char_type ;

      char_type operator[](ptrdiff_t pos) const
      {
         NOXPRECONDITION((size_t)pos < size()) ;
         return _str[pos] ;
      }

      template<typename S>
      S string() const { return S(_str ? _str : "") ; }

      size_t size() const { return _str ? str::len(_str) : 0 ; }
      constexpr bool empty() const { return !_str || !*_str ; }

      constexpr const char_type *c_str() const { return _str ; }
      constexpr operator const char_type *() const { return _str ; }

   protected:
      explicit constexpr basic_cstrptr_base(const char_type *s = nullptr) : _str(s) {}
      constexpr basic_cstrptr_base(const basic_cstrptr_base &other) : _str(other._str) {}

      void swap(basic_cstrptr_base &other) { std::swap(_str, other._str) ; }

      basic_cstrptr_base &operator=(const basic_cstrptr_base &) = default ;

   private:
      const char_type *_str ;
} ;

template<typename C>
struct basic_cstrptr<C, void> : basic_cstrptr_base<C> {
      typedef void deleter_type ;

      constexpr basic_cstrptr() = default ;
      constexpr basic_cstrptr(const basic_cstrptr &) = default ;
      constexpr basic_cstrptr(nullptr_t) : basic_cstrptr() {}

      template<typename S, enable_if_strchar_t<S, C, nullptr_t> = nullptr>
      constexpr basic_cstrptr(const S &s) :
         basic_cstrptr_base<C>(str::cstr(s))
      {}

      basic_cstrptr &operator=(const basic_cstrptr &) = default ;

      void swap(basic_cstrptr &other) { basic_cstrptr_base<C>::swap(other) ; }
} ;

template<typename C, typename Deleter>
struct basic_cstrptr : basic_cstrptr_base<C> {

      typedef Deleter deleter_type ;

      constexpr basic_cstrptr() {}
      constexpr basic_cstrptr(nullptr_t) : basic_cstrptr() {}
      explicit constexpr basic_cstrptr(C *str) : basic_cstrptr_base<C>(str) {}
      basic_cstrptr(basic_cstrptr &&other) { swap(other) ; }

      basic_cstrptr &operator=(basic_cstrptr &&other)
      {
         basic_cstrptr(std::move(other)).swap(*this) ;
         return *this ;
      }

      ~basic_cstrptr() { if (this->c_str()) _deleter(const_cast<C *>(this->c_str())) ; }

      void swap(basic_cstrptr &other) { basic_cstrptr_base<C>::swap(other) ; }
   private:
      static deleter_type _deleter ;
} ;

template<typename C, typename Deleter>
typename basic_cstrptr<C, Deleter>::deleter_type basic_cstrptr<C, Deleter>::_deleter ;

/***************************************************************************//**
 String traits for basic_cstrptr_base<C> and basic_cstrptr<C, D>
*******************************************************************************/
template<typename C>
struct string_traits<basic_cstrptr_base<C>> {
      typedef C                     char_type ;
      typedef basic_cstrptr_base<C> type ;
      typedef size_t                size_type ;
      typedef size_t                hash_type ;

      static size_type len(const type &s) { return s.size() ; }
      static constexpr const char_type *cstr(const type &s) { return s.c_str() ; }
      static hash_type hash(const type &s) { return string_traits<const char_type *>::hash(str::cstr(s)) ; }
} ;

template<typename C, typename D>
struct string_traits<basic_cstrptr<C, D> > : string_traits<basic_cstrptr_base<C> > {} ;

/*******************************************************************************
 Equality and ordering
*******************************************************************************/
template<typename C>
inline bool operator==(const basic_cstrptr_base<C> &left, const basic_cstrptr_base<C> &right)
{
   return left.c_str() == right.c_str() ||
      left.c_str() && right.c_str() && !ctype_traits<C>::strcmp(left.c_str(), right.c_str()) ;
}

/// basic_cstrptr comparison treates character as unsigned
template<typename C>
inline bool operator<(const basic_cstrptr_base<C> &left, const basic_cstrptr_base<C> &right)
{
   if (left.c_str() && right.c_str())
      return ctype_traits<C>::strcmp(left.c_str(), right.c_str()) < 0 ;
   else
      return left.c_str() < right.c_str() ;
}

// Define "derived" operators: !=, >, <=, >=
PCOMN_DEFINE_RELOP_FUNCTIONS(template<typename C>, basic_cstrptr_base<C>) ;

// Define global swap
PCOMN_DEFINE_SWAP(basic_cstrptr<P_PASS(C, D)>, template<typename C, typename D>) ;

/*******************************************************************************
 Typedefs for narrow and wide chars
*******************************************************************************/
typedef basic_cstrptr<char>     cstrptr ;
typedef basic_cstrptr<wchar_t>  cwstrptr ;

/*******************************************************************************
 ostream output
*******************************************************************************/
template<typename C>
inline std::basic_ostream<C> &operator<<(std::basic_ostream<C> &os, const basic_cstrptr_base<char> &s)
{
   return os << s.string<const char *>() ;
}

inline std::ostream &operator<<(std::ostream &os, const basic_cstrptr_base<wchar_t> &s)
{
   return narrow_output(os, s.c_str(), s.c_str() + s.size()) ;
}
} // end of namespace pcomn

namespace std {
/*******************************************************************************
 std::hash specialization for basic_cstrptr
*******************************************************************************/
template<typename D>
struct hash<pcomn::basic_cstrptr<char, D>> : pcomn::hash_fn<char *> {} ;
}

#endif /* __PCOMN_CSTRPTR_H */
