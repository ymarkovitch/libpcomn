/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_zstd.cpp
 COPYRIGHT    :   Yakov Markovitch, 2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   zstd/zdict wrappers.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   14 Oct 2019
*******************************************************************************/
#define ZSTD_STATIC_LINKING_ONLY
#include "pcomn_zstd.h"
#include "pcomn_utils.h"

namespace pcomn {
/*******************************************************************************
 zdict
*******************************************************************************/
zdict::zdict(const simple_cslice<std::string> &strings, size_t capacity) :
    zdict(train_from_strvector(strings, capacity), true, instantiate)
{}

zdict::zdict(const simple_cslice<strslice> &strings, size_t capacity) :
    zdict(train_from_strvector(strings, capacity), true, instantiate)
{}

iovec_t zdict::train(const void *sample_buffer, const simple_cslice<size_t> &sample_sizes, size_t capacity)
{
    PCOMN_ENSURE_ARG(sample_buffer) ;

    std::unique_ptr<char[]> dict_buf (new char[capacity]) ;
    const size_t dict_size =
        ensure_zdict(ZDICT_trainFromBuffer(dict_buf.get(), capacity,
                                           sample_buffer,
                                           sample_sizes.begin(), sample_sizes.size())) ;

    return make_iovec(dict_buf.release(), dict_size) ;
}

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

/*******************************************************************************
 zdict_cctx
*******************************************************************************/
zdict_cctx::zdict_cctx(const zdict &trained_dict, int clevel) :
    _id(trained_dict.id()),
    _clevel(clevel),
    _ctx(ensure_nonzero<std::bad_alloc>(ZSTD_createCCtx())),
    _dict(trained_dict.cdict(clevel))
{
    ensure_zstd(ZSTD_compressBegin_usingCDict(ctx(), dict())) ;
}

size_t zdict_cctx::compress_raw_block(void *dst, size_t dstsize,
                                      const void *src, size_t srcsize) const
{
    return ensure_zstd
        (ZSTD_compressBlock(ctx(), dst, dstsize, src, srcsize)) ;
}

} // end of namespace pcomn
