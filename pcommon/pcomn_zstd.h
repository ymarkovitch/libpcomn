/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#ifndef __PCOMN_ZSTD_H
#define __PCOMN_ZSTD_H
/*******************************************************************************
 FILE         :   pcomn_zstd.h
 COPYRIGHT    :   Yakov Markovitch, 2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   zstd/zdict wrappers.
 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   9 Oct 2019
*******************************************************************************/
/** @file
*******************************************************************************/
#include <pcomn_utils.h>
#include <pcomn_strslice.h>
#include <pcomn_vector.h>
#include <pcomn_meta.h>
#include <pcomn_except.h>
#include <pcomn_buffer.h>
#include <pcomn_safeptr.h>

#include <memory>

#include <zdict.h>
#include <zstd.h>

namespace pcomn {

/***************************************************************************//**
  Base ZSTD exception to indicate ZSTD API errors.
*******************************************************************************/
class zstd_error : public std::runtime_error {
    typedef std::runtime_error ancestor ;
public:
    using ancestor::ancestor ;
} ;

/***************************************************************************//**
  Exception to indicate dictionary API errors.
*******************************************************************************/
class zdict_error : public zstd_error {
    typedef zstd_error ancestor ;
public:
    using ancestor::ancestor ;
} ;

/*******************************************************************************
 ZSTD error checking.
*******************************************************************************/
/// Ensure the result of ZSTD compression/decompression API is not an error.
/// If @a retval is an error code, throw zstd_error, otherwise return it unchanged.
inline size_t zstd_ensure(size_t retval)
{
    PCOMN_THROW_IF(ZSTD_isError(retval), zstd_error, "%s", ZSTD_getErrorName(retval)) ;
    return retval ;
}

/// Ensure the result of ZDICT dictionary API is not an error.
/// If @a retval is an error code, throw zdict_error, otherwise return it unchanged.
inline size_t zdict_ensure(size_t retval)
{
    PCOMN_THROW_IF(ZDICT_isError(retval), zdict_error, "%s", ZDICT_getErrorName(retval)) ;
    return retval ;
}

/***************************************************************************//**
 ZStandard dictionary.
*******************************************************************************/
class zdict {
    PCOMN_NONCOPYABLE(zdict) ;
    PCOMN_NONASSIGNABLE(zdict) ;
public:
    /// Create a dictionary object from the memory containing an already trained
    /// dictionary.
    explicit zdict(const pcomn::iovec_t &trained_dict) :
        _trained_dict(trained_dict),

        _id(ensure_nonzero<zdict_error>
            (ZDICT_getDictID(buf::cdata(_trained_dict), buf::size(_trained_dict)),
             "Invalid dictionary passed to pcomn::zdict constructor")),

        _owned(false)
    {}

    /// Train
    zdict(const void *sample_buffer, const simple_cslice<size_t> &sample_sizes, size_t capacity) :
        _trained_dict(train(sample_buffer, sample_sizes, capacity)),
        _id(ZDICT_getDictID(buf::cdata(_trained_dict), buf::size(_trained_dict))),
        _owned(true)
    {}

    explicit zdict(const simple_cslice<std::string> &) ;
    explicit zdict(const simple_cslice<strslice> &) ;

    ~zdict()
    {
        if (_owned)
            delete [] static_cast<char*>(buf::data(_trained_dict)) ;
    }

    /// Get the dictionary ID
    unsigned id() const { return _id ; }

private:
    const pcomn::iovec_t _trained_dict ;
    const unsigned       _id ;
    const bool           _owned ; /* Is destruction needed */

private:
    static __noinline pcomn::iovec_t train(const void *sample_buffer,
                                           const simple_cslice<size_t> &sample_sizes,
                                           size_t capacity)
    {
        PCOMN_ENSURE_ARG(sample_buffer) ;

        std::unique_ptr<char[]> dict_buf (new char[capacity]) ;
        const size_t dict_size =
            zdict_ensure(ZDICT_trainFromBuffer(dict_buf.get(), capacity,
                                               sample_buffer,
                                               sample_sizes.begin(), sample_sizes.size())) ;

        return make_iovec(dict_buf.get(), dict_size) ;
    }
} ;

} // end of namespace pcomn

#endif /* __PCOMN_ZSTD_H */
