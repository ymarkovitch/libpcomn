/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_ALLOCA_H
#define __PCOMN_ALLOCA_H
/*******************************************************************************
 FILE         :   pcomn_alloca.h
 COPYRIGHT    :   Yakov Markovitch, 1999-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   alloca() function declaration for different compilers

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   24 Dec 1999
*******************************************************************************/
#include <malloc.h>
#include <stdlib.h>

/** Call alloca() in a typesafe manner.
 */
#define P_ALLOCA(type, nitems) (static_cast<type *>(alloca(sizeof(type) * (nitems))))

#ifdef __cplusplus
#include <memory>
#include <type_traits>

/// Define a raw buffer either on the stack or in the heap, depending on size and
/// threshold
///
/// @param name      The name of the declared variable.
/// @param type      The pointer base type of the declared variable; the macro will declare
///   type *name variable.
/// @param nitems    The number of elements in the buffer; the macro allocates a buffer of
///   sizeof(type)*nitems bytes, aligned to alignmof(type).
/// @param threshold Maximum stack allocation size in @em bytes; if the size of resulting
///   buffer is greater, the buffer is allocated in the heap.
///
/// @note Uses alloca() to allocate memory on the stack.
///
#define P_FAST_BUFFER(name, itemtype, nitems, threshold)                \
   const size_t _var_##__LINE__##name##_count_ = (nitems) ;             \
   const std::unique_ptr<typename std::aligned_storage<sizeof(itemtype), std::alignment_of<itemtype >::value>::type> \
   _var_##__LINE__##name##_guard_                                       \
   (_var_##__LINE__##name##_count_ * sizeof(itemtype) <= (threshold)    \
    ? nullptr                                                           \
    : new typename std::aligned_storage<sizeof(itemtype), std::alignment_of<itemtype >::value>::type[_var_##__LINE__##name##_count_]) ; \
   itemtype *name = reinterpret_cast<itemtype *>(_var_##__LINE__##name##_guard_.get()) ; \
   if (!name && _var_##__LINE__##name##_count_) name = P_ALLOCA(itemtype, _var_##__LINE__##name##_count_) ;

#endif // __cplusplus

#endif /* __PCOMN_ALLOCA_H */
