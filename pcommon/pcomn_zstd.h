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
#include <pcomn_handle.h>

#include <memory>

#define ZSTD_STATIC_LINKING_ONLY
#include <zdict.h>
#include <zstd.h>

namespace std {
template<> struct default_delete<ZSTD_CCtx>  { void operator()(ZSTD_CCtx *x) const { ZSTD_freeCCtx(x) ; } } ;
template<> struct default_delete<ZSTD_CDict> { void operator()(ZSTD_CDict *x) const { ZSTD_freeCDict(x) ; } } ;
} // end of namespace std

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

    zdict(const simple_cslice<std::string> &strings, size_t capacity) :
        zdict(train_from_strvector(strings, capacity), true, instantiate)
    {}

    zdict(const simple_cslice<strslice> &strings, size_t capacity) :
        zdict(train_from_strvector(strings, capacity), true, instantiate)
    {}

    ~zdict()
    {
        if (_owned)
            delete [] static_cast<char*>(buf::data(_trained_dict)) ;
    }

    /// Get the dictionary ID
    unsigned id() const { return _id ; }

    const void *data() const { return buf::cdata(_trained_dict) ; }
    size_t size() const { return buf::size(_trained_dict) ; }

private:
    const iovec_t  _trained_dict ;
    const unsigned _id ;
    const bool     _owned ; /* Is destruction needed */

private:
    /// Use a knowinly valid dictionary.
    zdict(const iovec_t &trained_dict, bool owned, Instantiate) :
        _trained_dict(trained_dict),
        _id(ZDICT_getDictID(buf::cdata(_trained_dict), buf::size(_trained_dict))),
        _owned(owned)
    {
        NOXCHECK(_id) ;
    }

    static __noinline iovec_t train(const void *sample_buffer,
                                    const simple_cslice<size_t> &sample_sizes,
                                    size_t capacity)
    {
        PCOMN_ENSURE_ARG(sample_buffer) ;

        std::unique_ptr<char[]> dict_buf (new char[capacity]) ;
        const size_t dict_size =
            zdict_ensure(ZDICT_trainFromBuffer(dict_buf.get(), capacity,
                                               sample_buffer,
                                               sample_sizes.begin(), sample_sizes.size())) ;

        return make_iovec(dict_buf.release(), dict_size) ;
    }

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
    __noinline
    zdict_cctx(const zdict &trained_dict, int clevel = 3) :
        _id(trained_dict.id()),
        _clevel(clevel),
        _ctx(ensure_nonzero<std::bad_alloc>(ZSTD_createCCtx())),
        _dict(ZSTD_createCDict(trained_dict.data(), trained_dict.size(), clevel))
    {
        zstd_ensure(ZSTD_compressBegin_usingCDict(ctx(), dict())) ;
    }

    /// Get the dictionary ID
    unsigned id() const { return _id ; }

    int compression_level() const { return _clevel ; }

private:
    const unsigned     _id ;
    const int          _clevel ;

    const std::unique_ptr<ZSTD_CCtx>  _ctx ;
    const std::unique_ptr<ZSTD_CDict> _dict ;

private:
    ZSTD_CCtx *ctx() const { return _ctx.get() ; }
    ZSTD_CDict *dict() const { return _dict.get() ; }
} ;

/*******************************************************************************
 zdict
*******************************************************************************/
template<typename T>
iovec_t zdict::train_from_strvector(const T &strings, size_t capacity)
{
    size_t total_size = 0 ;
    const size_t nempty_count = std::count_if(std::begin(strings), std::end(strings), [&](const auto &s)
    {
        const size_t sz = std::size(s) ;
        total_size += sz ;
        return sz ;
    }) ;
    if (!total_size)
        return {} ;

    const std::unique_ptr<size_t> buf (new size_t[nempty_count + (total_size+sizeof(size_t)-1)/sizeof(size_t)]) ;
    size_t *sample_sz = buf.get() ;
    char *sample_data = (char *)(sample_sz + nempty_count) ;

    for (const auto &s: strings)
        if (const size_t ssz = std::size(s))
        {
            memcpy(sample_data, std::data(s), ssz) ;
            sample_data += ssz ;
            *sample_sz++ = ssz ;
        }

    return train(sample_sz, {buf.get(), sample_sz}, capacity) ;
}

} // end of namespace pcomn

#endif /* __PCOMN_ZSTD_H */
