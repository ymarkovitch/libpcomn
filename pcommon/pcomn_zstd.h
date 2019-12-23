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
#include <pcomn_strslice.h>
#include <pcomn_vector.h>
#include <pcomn_meta.h>
#include <pcomn_except.h>
#include <pcomn_buffer.h>

#include <memory>

#include <zdict.h>
#include <zstd.h>

namespace pcomn {

template<typename T> struct zstd_free ;

template<typename T>
using zstd_handle = std::unique_ptr<T, zstd_free<T>> ;

template<> struct zstd_free<ZSTD_CCtx>  { void operator()(ZSTD_CCtx *x) const { ZSTD_freeCCtx(x) ; } } ;
template<> struct zstd_free<ZSTD_DCtx>  { void operator()(ZSTD_DCtx *x) const { ZSTD_freeDCtx(x) ; } } ;
template<> struct zstd_free<ZSTD_CDict> { void operator()(ZSTD_CDict *x) const { ZSTD_freeCDict(x) ; } } ;
template<> struct zstd_free<ZSTD_DDict> { void operator()(ZSTD_DDict *x) const { ZSTD_freeDDict(x) ; } } ;

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
inline size_t ensure_zstd(size_t retval)
{
    PCOMN_THROW_IF(ZSTD_isError(retval), zstd_error, "%s", ZSTD_getErrorName(retval)) ;
    return retval ;
}

/// Ensure the result of ZDICT dictionary API is not an error.
/// If @a retval is an error code, throw zdict_error, otherwise return it unchanged.
inline size_t ensure_zdict(size_t retval)
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
    explicit zdict(const iovec_t &trained_dict) :
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

    zdict(const simple_cslice<std::string> &strings, size_t capacity) ;
    zdict(const simple_cslice<strslice> &strings, size_t capacity) ;

    ~zdict()
    {
        if (_owned)
            delete [] static_cast<char*>(buf::data(_trained_dict)) ;
    }

    /// Get the dictionary ID automatically stored by the dictionary builder.
    /// See `--dictId` command line option of zstd.
    ///
    unsigned id() const { return _id ; }

    digest128_t digest() const ;

    const void *data() const { return buf::cdata(_trained_dict) ; }
    size_t size() const { return buf::size(_trained_dict) ; }

    zstd_handle<ZSTD_CDict> cdict(int clevel = 3) const
    {
        return {ZSTD_createCDict(data(), size(), clevel), {}} ;
    }

    zstd_handle<ZSTD_DDict> ddict() const
    {
        return {ZSTD_createDDict(data(), size()), {}} ;
    }

private:
    const iovec_t       _trained_dict ;
    const unsigned      _id ;
    const bool          _owned ; /* Is destruction needed */
    mutable t1ha2hash_t _digest ;

private:
    /// Use a knowinly valid dictionary.
    zdict(const iovec_t &trained_dict, bool owned, Instantiate) :
        _trained_dict(trained_dict),
        _id(ZDICT_getDictID(buf::cdata(_trained_dict), buf::size(_trained_dict))),
        _owned(owned)
    {
        NOXCHECK(_id) ;
    }

    static iovec_t train(const void *sample_buffer,
                         const simple_cslice<size_t> &sample_sizes,
                         size_t capacity) ;

    template<typename T>
    static iovec_t train_from_strvector(const T &strings, size_t capacity) ;
} ;

/***************************************************************************//**
 ZStandard compression context for dictionary compression.
*******************************************************************************/
class zdict_cctx {
    PCOMN_NONCOPYABLE(zdict_cctx) ;
    PCOMN_NONASSIGNABLE(zdict_cctx) ;
public:
    zdict_cctx(const zdict &trained_dict, int clevel = 3) ;

    /// Get the dictionary ID
    unsigned id() const { return _id ; }

    int compression_level() const { return _clevel ; }

    template<typename T>
    auto compress_frame(T &&src_data, const iovec_t &dest_buf) const
        -> type_t<decltype(buf::size(src_data)),
                  decltype(std::data(std::forward<T>(src_data)))>
    {
        return ensure_zstd
            (ZSTD_compress_usingCDict(ctx(),
                                      std::data(dest_buf), buf::size(dest_buf),
                                      std::data(std::forward<T>(src_data)), buf::size(std::forward<T>(src_data)),
                                      dict())) ;
    }

    template<typename T>
    auto compress_frame_nodict(T &&src_data, const iovec_t &dest_buf) const
        -> type_t<decltype(buf::size(src_data)),
                  decltype(std::data(std::forward<T>(src_data)))>
    {
        return ensure_zstd
            (ZSTD_compressCCtx(ctx(),
                               std::data(dest_buf), buf::size(dest_buf),
                               std::data(std::forward<T>(src_data)), buf::size(std::forward<T>(src_data)),
                               _clevel)) ;
    }

    template<typename T>
    auto compress_block(T &&src_data, const iovec_t &dest_buf) const
        -> type_t<decltype(buf::size(src_data)),
                  decltype(std::data(std::forward<T>(src_data)))>
    {
        return compress_raw_block(std::data(dest_buf), buf::size(dest_buf),
                                  std::data(std::forward<T>(src_data)), buf::size(std::forward<T>(src_data))) ;
    }

private:
    const unsigned  _id ;
    const int       _clevel ;

    const zstd_handle<ZSTD_CCtx>  _ctx ;
    const zstd_handle<ZSTD_CDict> _dict ;

private:
    ZSTD_CCtx *ctx() const { return _ctx.get() ; }
    ZSTD_CDict *dict() const { return _dict.get() ; }

    size_t compress_raw_block(void *dst, size_t dstsize,
                              const void *src, size_t srcsize) const ;
} ;

} // end of namespace pcomn

#endif /* __PCOMN_ZSTD_H */
