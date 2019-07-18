/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_ALLOCA_H
#define __PCOMN_ALLOCA_H
/*******************************************************************************
 FILE         :   pcomn_alloca.h
 COPYRIGHT    :   Yakov Markovitch, 1999-2018. All rights reserved.
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
///   sizeof(type)*nitems bytes, aligned to alignof(type).
/// @param threshold_bytes Maximum stack allocation size in _bytes_; if the size of
///   resulting buffer is greater, the buffer is allocated in the heap.
///
/// @note Uses alloca() to allocate memory on the stack.
///
#define P_FAST_BUFFER(name, itemtype, nitems, threshold_bytes)          \
   const size_t _var_##__LINE__##name##_count_ = (nitems) ;             \
   const size_t _var_##__LINE__##name##_acnt_ =                          \
      ((_var_##__LINE__##name##_count_ * sizeof(itemtype)) > (threshold_bytes) ? 0 : _var_##__LINE__##name##_count_) ; \
                                                                        \
   itemtype * const _var_##__LINE__##name##_adata_ = P_ALLOCA(itemtype, _var_##__LINE__##name##_acnt_) ; \
                                                                        \
   const std::unique_ptr<std::aligned_storage_t<sizeof(itemtype), std::alignment_of_v<itemtype>>[]> \
   _var_##__LINE__##name##_guard_                                       \
   (_var_##__LINE__##name##_acnt_ || !_var_##__LINE__##name##_count_ ? nullptr \
    : new std::aligned_storage_t<sizeof(itemtype), std::alignment_of_v<itemtype>>[_var_##__LINE__##name##_count_]) ; \
                                                                        \
   itemtype * const name = (!_var_##__LINE__##name##_count_ || _var_##__LINE__##name##_guard_) \
      ? reinterpret_cast<itemtype *>(_var_##__LINE__##name##_guard_.get()) \
      : _var_##__LINE__##name##_adata_ ;

#endif // __cplusplus

#endif /* __PCOMN_ALLOCA_H */
