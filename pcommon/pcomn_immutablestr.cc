/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#pragma once
/*******************************************************************************
 FILE         :   pcomn_immutablestr.cc
 COPYRIGHT    :   Yakov Markovitch, 2006-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   This is an internal header file.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   14 Nov 2006
*******************************************************************************/

namespace pcomn {

/*******************************************************************************
 refcounted_strdata
*******************************************************************************/
template<typename C>
const refcounted_strdata<C> refcounted_strdata<C>::zero = {} ;

/*******************************************************************************
 refcounted_storage
*******************************************************************************/
template<typename C, class A>
inline
void *refcounted_storage<C,A>::do_alloc(size_type &char_count) const
{
    NOXCHECK(char_count) ;
    const size_type allocated_items = aligner_count(char_count) ;
    void * const allocated = actual_allocator(*this).allocate(allocated_items, 0) ;
    char_count = allocated_count(allocated_items) ;
    return allocated ;
}

template<typename C, class A>
inline typename refcounted_storage<C,A>::data_type *
refcounted_storage<C,A>::create_str_data(size_type &char_count) const
{
    NOXCHECK(char_count) ;
    const size_type requested_count = char_count ;
    // Don't forget place for trailing zero
    size_type actual_size = char_count + 1 ;
    data_type * const d = reinterpret_cast<data_type *>(do_alloc(actual_size)) ;
    char_count = actual_size - 1 ;
    d->_refcount.reset(1) ;
    d->_size = requested_count ;
    // Trailing zero (C string compatible)
    *d->end() = value_type() ;
    return d ;
}

template<typename C, class A>
refcounted_storage<C,A>::refcounted_storage(const value_type *source, size_type len,
                                            const allocator_type &a) :
   allocator_type(a),
   _data(const_cast<value_type *>(data_type::zero.begin()))
{
   if (!len) return ;

   size_type capacity = len ;
   raw_copy(source, source + len, _data = create_str_data(capacity)->begin()) ;
}


template<typename C, class A>
refcounted_storage<C,A>::refcounted_storage(size_type len, value_type c, const allocator_type &a) :
   allocator_type(a),
   _data(const_cast<value_type *>(data_type::zero.begin()))
{
   if (!len) return ;

   size_type capacity = len ;
   data_type * const d = create_str_data(capacity) ;
   pcomn::raw_fill(d->begin(), d->end(), c) ;
   _data = d->begin() ;
}

/*******************************************************************************
 mutable_strbuf
*******************************************************************************/
template<typename C>
void mutable_strbuf<C>::recapacitate(size_type requested_capacity)
{
   NOXCHECK(requested_capacity > _capacity) ;
   // _capacity is always odd (by design, due to alignement + trailing 0)
   size_type actual_capacity =
      std::max(_capacity + (_capacity + 1) / 2, requested_capacity) + 1 ;
   data_type *old_data = &storage_type::str_data() ;
   data_type *new_data = static_cast<data_type *>(this->do_alloc(actual_capacity)) ;
   memcpy((void *)new_data, (void *)old_data, this->allocated_size(old_data->_size + 1)) ;
   this->storage_type::_data = new_data->_begin ;
   // "-1" stands for the trailing zero
   _capacity = actual_capacity - 1 ;
   new_data->_refcount.reset(1) ;
   if (old_data->_size)
   {
      NOXCHECK(old_data->_refcount.count() == 1) ;
      this->do_dealloc(old_data) ;
   }
}

/*******************************************************************************
 shared_string
*******************************************************************************/
template<typename C>
constexpr typename shared_string<C>::size_type npos ;

template<typename C>
template<class Predicate>
inline typename shared_string<C>::size_type
shared_string<C>::__find_first(const value_type *s, size_type pos, size_type n) const
{
    Predicate predicate ;
    if (pos > size() || !n)
        return npos ;
    const const_iterator start (begin()) ;
    const const_iterator finish (end()) ;
    for (const_iterator current (start + pos) ; current != finish ; ++current)
        if (predicate(traits_type::find(s, n, *current)))
            return current - start ;
    return npos ;
}

template<typename C>
template<class Predicate>
inline typename shared_string<C>::size_type
shared_string<C>::__find_last(const value_type *s, size_type pos, size_type n) const
{
    Predicate predicate ;
    const size_type sz = size() ;
    const size_type startpos = std::min(pos, sz) ;
    if (!startpos || !n)
        return npos ;

    const const_iterator finish (begin()) ;
    const_iterator current (finish + (startpos - 1)) ;
    while (!predicate(traits_type::find(s, n, *current)))
    {
        if (current == finish)
            return npos ;
        --current ;
    }
    return current - finish ;
}

template<typename C>
typename shared_string<C>::size_type
shared_string<C>::find_first_of(const value_type *s, size_type pos, size_type n) const
{
    return __find_first<identity>(s, pos, n) ;
}

template<typename C>
typename shared_string<C>::size_type
shared_string<C>::find_last_of(const value_type* s, size_type pos, size_type n) const
{
    return __find_last<identity>(s, pos, n) ;
}

template<typename C>
typename shared_string<C>::size_type
shared_string<C>::find_first_not_of(const value_type *s, size_type pos, size_type n) const
{
    return __find_first<std::logical_not<const char_type *>>(s, pos, n) ;
}

template<typename C>
typename shared_string<C>::size_type
shared_string<C>::find_last_not_of(const value_type* s, size_type pos, size_type n) const
{
    return __find_last<std::logical_not<const char_type *>>(s, pos, n) ;
}

template<typename C>
__noreturn __cold void shared_string<C>::bad_pos(size_type pos) const
{
   char buf[96] ;
   sprintf(buf, "Position %u is out of range for shared string of size %u.", pos, size()) ;
   throw std::out_of_range(buf) ;
}

} // end of namespace pcomn
