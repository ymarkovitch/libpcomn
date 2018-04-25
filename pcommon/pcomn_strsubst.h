/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_STRSUBST_H
#define __PCOMN_STRSUBST_H
/*******************************************************************************
 FILE         :   pcomn_strsubst.h
 COPYRIGHT    :   Yakov Markovitch, 2007-2017. All rights reserved.

 DESCRIPTION  :   String substitution a la Perl templates or Python.Template class.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   24 Dec 2007
*******************************************************************************/
/** @file
    Provides a set of function and classes that implement string substitution a la
    Perl templates or Python Template class.

    The following rules for string substitution apply:

    @li A template string contains placeholders, introduced with the $ character.

    @li $$ is an escape; it is replaced with a single $

    @li $identifier names a substitution placeholder matching a mapping key of
        "identifier". By default, "identifier" must spell a C identifier (starting with
        a latin character or an underscore, then arbitrary sequence of lating
        characters, underscores and digits). The first non-identifier character after
        the $ character terminates the placeholder.
        ${identifier} notation is used to separate a placeholder from

    @li "${identifier}" is equivalent to "$identifier". It is required when valid
        identifier characters follow the placeholder but are not part of the placeholder,
        such as "${noun}isation".
*******************************************************************************/
#include <pcomn_iodevice.h>
#include <pcomn_handle.h>
#include <pcomn_string.h>
#include <pcomn_strslice.h>
#include <pcomn_strnum.h>
#include <pcomn_integer.h>
#include <pcomn_hashclosed.h>
#include <pcomn_algorithm.h>
#include <pcomn_binstream.h>

#include <functional>
#include <type_traits>
#include <string>
#include <algorithm>
#include <memory>
#include <utility>
#include <new>

#include <ctype.h>

namespace pcomn {
namespace tpl {
/// @cond
namespace detail {

struct _PCOMNEXP output : public std::binary_function<void *, size_t, ssize_t> {
      virtual ssize_t operator()(const void *, size_t) const = 0 ;
      virtual ~output() {}
} ;

template<typename OutputDevice>
struct owriter : output {
      owriter(OutputDevice device) : _device(device) {}

      ssize_t operator()(const void *data, size_t size) const
      {
         return io::write_data(_device, data, size) ;
      }
   private:
      OutputDevice _device ;
} ;

template<typename Device>
struct out : owriter<Device &> { out(Device &device) : owriter<Device &>(device) {} } ;
template<typename F>
struct out<F *> : owriter<F *> { out(F *device) : owriter<F *>(device) {} } ;
template<>
struct out<int> : owriter<int> { out(int fd) : owriter<int>(fd) {} } ;
template<typename T>
struct out<safe_handle<T> > : owriter<typename safe_handle<T>::handle_type> {
      typedef typename safe_handle<T>::handle_type handle_type ;

      out(handle_type h) : owriter<handle_type>(h) {}
} ;

inline ssize_t format(const output &out, const strslice &s)
{
   return out(s.begin(), s.size()) ;
}

inline ssize_t format(const output &out, char c)
{
   return out(&c, 1) ;
}

template<typename I>
inline typename if_integer<I, ssize_t>::type format(const output &out, I num)
{
   char buf[64] ;
   const typename std::conditional<int_traits<I>::is_signed, int64_t,  uint64_t>::type i = num ;
   numtostr(i, buf) ;
   return out(buf, strlen(buf)) ;
}

inline ssize_t format(const output &out, bool v)
{
   return format(out, (int)v) ;
}
} // end of namespace pcomn::tpl::detail
/// @endcond

/******************************************************************************/
/** String template substitutions mapping.

 Maps name:value or name:function.
*******************************************************************************/
class substitution_map {
      PCOMN_NONCOPYABLE(substitution_map) ;
      PCOMN_NONASSIGNABLE(substitution_map) ;

   public:
      _PCOMNEXP substitution_map() ;
      _PCOMNEXP ~substitution_map() throw() ;

      bool empty() const { return _replacement_map.empty() ; }

      void swap(substitution_map &other) { _replacement_map.swap(other._replacement_map) ; }

      /// Set substitution value for @a placeholder.
      /// @param placeholder
      /// @param value        Replacement string
      _PCOMNEXP substitution_map &operator()(const strslice &placeholder, const strslice &value) ;

      /// Set substitution value for undefined placeholder.
      /// @param value        Replacement string
      _PCOMNEXP substitution_map &operator()(const strslice &value) ;

      /// Set substitution value for @a placeholder.
      /// @param placeholder
      /// @param value        Integer or boolean value; immediately converted to a
      /// replacement string.
      template<typename V>
      std::enable_if_t<std::is_arithmetic<V>::value, substitution_map &>
      operator()(const strslice &placeholder, const V &value)
      {
         set_substval(strslice(placeholder), value) ;
         return *this ;
      }

      /// Set substitution for @a placeholder as a reference to a variable.
      /// @param placeholder
      /// @param vref         Reference to a variable.
      ///
      /// The value the variable has at the moment of subst() call will be substituted
      /// i.e., two successive subst() calls may give different results if a value of a
      /// variable @a valref references to is changed between the calls.
      template<typename V>
      substitution_map &operator()(const strslice &placeholder,
                                   const std::reference_wrapper<V> &vref) ;

      /// Set substitution for @a placeholder as a function.
      ///
      /// The result of @a valfn call will be substituted for @a placeholder.
      template<typename V>
      substitution_map &operator()(const strslice &placeholder, const std::function<V()> &valfn) ;

      /// Set substitution as a function for undefined placeholder.
      ///
      /// The result of @a valfn call will be substituted for @a placeholder.
      template<typename V>
      _PCOMNEXP substitution_map &operator()(const std::function<V(const strslice&)> &valfn) ;

      template<typename F>
      std::enable_if_t<std::is_bind_expression<F>::value, substitution_map &>
      operator()(const strslice &placeholder, const F &bindexpr)
      {
         typedef typename F::result_type V ;
         return (*this)(placeholder, std::function<V()>(bindexpr)) ;
      }

      /// Make substitutions for a template specified by a pair of input iterator.
      /// @param s               Substitution map.
      /// @param template_begin  Start of a string template.
      /// @param template_end    End of a string template.
      /// @param output
      ///
      /// The function scans the input sequence, asssuming it is a string template,
      /// makes all substitututions according to substitution_map, and outputs
      /// the resulting sequence into the output device.
      template<typename InputIterator, typename OutputDevice>
      friend OutputDevice &subst(const substitution_map &s,
                                 InputIterator template_begin, InputIterator template_end,
                                 OutputDevice &output)
      {
         istream_over_iterator<InputIterator> input (template_begin, template_end) ;
         s.substitute(input, output) ;
         return output ;
      }

      /// Make substitutions in a template specified by a string.
      template<typename InputDevice, typename OutputDevice>
      friend
      std::enable_if_t<std::is_convertible<InputDevice, strslice>::value, OutputDevice &>
      subst(const substitution_map &s, const InputDevice &template_string, OutputDevice &output)
      {
         strslice input (template_string) ;
         s.substitute(input, output) ;
         return output ;
      }

      /// Make substitutions in a template specified by any readable iodevice (i.e. such
      /// with pcomn::io::reader<> defined).
      template<typename InputDevice, typename OutputDevice>
      friend
      std::enable_if_t<(!std::is_pod<InputDevice>::value && !std::is_convertible<InputDevice, strslice>::value),
                       OutputDevice &>
      subst(const substitution_map &s, InputDevice &input, OutputDevice &output)
      {
         s.substitute(input, output) ;
         return output ;
      }

      /// @overload
      template<typename InputDevice, typename OutputDevice>
      friend
      std::enable_if_t<(std::is_pod<InputDevice>::value && !std::is_convertible<InputDevice, strslice>::value),
                       OutputDevice &>
      subst(const substitution_map &s, InputDevice input, OutputDevice &output)
      {
         s.substitute(input, output) ;
         return output ;
      }

   private:
      struct replacement_function {
            virtual ~replacement_function() {}
            virtual const strslice name() const = 0 ;
            virtual ssize_t write(detail::output &) const = 0 ;
      } ;

      struct replacement_string : replacement_function {
            replacement_string(const strslice &name, const strslice &value) :
               _namesize(name.size())
            {
               memcpy(_data, name.begin(), _namesize) ;
               char *v = _data + _namesize ;
               *v++ = 0 ;
               memcpy(v, value.begin(), value.size()) ;
               v[value.size()] = 0 ;
            }

            const strslice name() const { return strslice(_data + 0, _data + _namesize) ; }
            const char *value() const { return _data + _namesize + 1 ; }

            ssize_t write(detail::output &out) const
            {
               const char * const v = value() ;
               return out(v, strlen(v)) ;
            }

            static void *operator new(size_t sz, const strslice &name, const strslice &value)
            {
               return ::operator new (sz + name.size() + value.size() + 1) ;
            }
            static void operator delete(void *p, const strslice &, const strslice &) { ::operator delete (p) ; }
            static void operator delete(void *p, size_t) { ::operator delete (p) ; }

         private:
            size_t _namesize ;
            char   _data[1] ;
      } ;

      template<typename T>
      struct replacement_indirect : replacement_function {
            replacement_indirect(const strslice &nm, const T &vref) : _vref(vref)
            {
               memcpy(_name, nm.begin(), nm.size()) ;
               _name[nm.size()] = 0 ;
            }

            const strslice name() const { return _name ; }

            static void *operator new(size_t sz, const strslice &name) { return ::operator new (sz + name.size()) ; }
            static void operator delete(void *p, size_t, const strslice &) { ::operator delete (p) ; }
            static void operator delete(void *p, size_t) { ::operator delete (p) ; }
         protected:
            T     _vref ;
         private:
            char  _name[1] ;
      } ;

      template<typename V>
      struct replacement_fn : replacement_indirect<std::function<V()> > {
            replacement_fn(const strslice &nm, const std::function<V()> &fn) :
               replacement_indirect<std::function<V()> >(nm, fn)
            {}
            ssize_t write(detail::output &out) const { return detail::format(out, this->_vref()) ; }
      } ;

      template<typename T>
      struct replacement_ref : replacement_indirect<T &> {
            replacement_ref(const strslice &nm, T &vref) : replacement_indirect<T &>(nm, vref) {}
            ssize_t write(detail::output &out) const { return detail::format(out, this->_vref) ; }
      } ;

      struct replacement_default {
            virtual ~replacement_default() {}
            virtual ssize_t write(const strslice &placeholder, detail::output &) const = 0 ;
      } ;

      struct replacement_defstr : replacement_default {
            replacement_defstr(const strslice &value):_value(value.begin(), value.end()) {}
            ssize_t write(const strslice&, detail::output &out) const
            {
               return out(&*_value.begin(), _value.size()) ;
            }
         private:
            const std::vector<char> _value ;
      } ;

      template<typename T>
      struct replacement_deffn : replacement_default {
            replacement_deffn(const T &vref):_vref(vref) {}
            ssize_t write(const strslice &placeholder, detail::output &out) const
            {
               return detail::format(out, this->_vref(placeholder)) ;
            }
         private:
            T  _vref ;
      } ;

      enum State { S_TEXT, S_PLACEHOLDER_START, S_PLACEHOLDER, S_PLACEHOLDER_QUOTED, S_COMMENTS } ;
      struct local_state {
         State             state ;
         char *            textbuf_begin ;
         char *            textbuf_end ;
         char *            endp ;
         std::vector<char>&placeholder ;
         detail::output &  out ;

         // Flush the text buffer
         void flush_text()
         {
            if (endp == textbuf_begin)
               return ;
            out(textbuf_begin, endp - textbuf_begin) ;
            endp = textbuf_begin ;
         }
         void finish_placeholder(int nextchar)
         {
            placeholder.clear() ;

            if (nextchar == '$' || nextchar == -1)
               state = S_PLACEHOLDER_START ;
            else
            {
               *endp++ = nextchar ;
               state = S_TEXT ;
            }
         }
      } ;

      template<typename V>
      void set_substval(const strslice &placeholder, const V &value) ;
      void replace_substfn(replacement_function *&&value) ;

      // Scan the input sequence, asssuming it is a string template, makes all
      // substitutution, and outputs the resulting sequence into the output device.
      template<typename OutputDevice, typename InputDevice>
      const substitution_map &substitute(InputDevice &input, OutputDevice &output) const ;

      _PCOMNEXP bool consume_char(int c, local_state &local) const ;

      // Output placeholder substitution value to out; if there is no such placeholder
      // registered, output the placeholder enclosed in pfx and sfx.
      _PCOMNEXP void commit_substitution(detail::output &out, const strslice &placeholder,
                                         const char *prefix, const char *suffix = "") const ;

   private:
      typedef closed_hashtable<replacement_function *, extract_name> replacement_map ;

      replacement_map _replacement_map ;
      std::unique_ptr<replacement_default> _replacement_def ;
} ;

template<typename V>
__noinline void substitution_map::set_substval(const strslice &placeholder, const V &value)
{
   std::string substval ;
   detail::out<std::string> outval (substval) ;
   detail::format(outval, value) ;
   (*this)(placeholder, substval) ;
}

template<typename V>
__noinline substitution_map &substitution_map::operator()(const strslice &placeholder,
                                                          const std::function<V()> &valfn)
{
   replace_substfn(std::move(new (placeholder) replacement_fn<V>(placeholder, valfn))) ;
   return *this ;
}

template<typename V>
__noinline substitution_map &substitution_map::operator()(const strslice &placeholder,
                                                          const std::reference_wrapper<V> &vref)
{
   replace_substfn(std::move(new (placeholder) replacement_ref<V>(placeholder, vref))) ;
   return *this ;
}

inline substitution_map &substitution_map::operator()(const strslice &value)
{
   _replacement_def.reset(new replacement_defstr(value)) ;
   return *this ;
}

template<typename V>
__noinline substitution_map &substitution_map::operator()(const std::function<V(const strslice&)> &valfn)
{
   _replacement_def.reset(new replacement_deffn<std::function<V(const strslice&)> >(valfn)) ;
   return *this ;
}

template<typename OutputDevice, typename InputDevice>
const substitution_map &substitution_map::substitute(InputDevice &input, OutputDevice &output) const
{
   char textbuf[4096] ;
   detail::out<OutputDevice> textout (output) ;
   std::vector<char> placeholder ;
   local_state local = { S_TEXT, textbuf, textbuf + sizeof textbuf, textbuf, placeholder, textout } ;

   while (consume_char(io::get_char(input), local)) ;
   return *this ;
}

/******************************************************************************/
/** Global swap(substitution_map&,substitution_map&)
*******************************************************************************/
PCOMN_DEFINE_SWAP(substitution_map) ;

} // end of namespace pcomn::tpl
} // end of namespace pcomn

#endif /* __PCOMN_STRSUBST_H */
