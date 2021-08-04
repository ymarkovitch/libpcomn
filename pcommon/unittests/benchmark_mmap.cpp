/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   unittest_mmap.cpp
 COPYRIGHT    :   Yakov Markovitch, 2007-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test for memory-mapping classes.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Aug 2007
*******************************************************************************/
#include <pcomn_mmap.h>
#include <pcomn_handle.h>
#include <pcomn_rawstream.h>
#include <pcomn_sys.h>
#include <pcomn_hash.h>
#include <pcomn_calgorithm.h>

#include <memory>
#include <future>
#include <thread>
#include <experimental/filesystem>

#include <sys/stat.h>

using namespace pcomn ;
using namespace pcomn::path ;
namespace fs = std::experimental::filesystem::v1 ;

constexpr size_t head_size = 64 ;
constexpr size_t tail_size = 256 ;

std::vector<std::string> all_files()
{
    std::vector<std::string> result ;
    for (const auto &f: fs::recursive_directory_iterator(".", fs::directory_options::skip_permission_denied))
        if (f.status().type() == fs::file_type::regular)
            result.push_back(f.path().string()) ;
    return result ;
}

unipair<size_t> run_bench()
{
    hash_combinator h ;
    size_t count = 0 ;

    std::vector<std::string> files (all_files()) ;
    pcomn::sort(files) ;

    for (const std::string &f: files)
    {
        pcomn::PMemMapping m (f.c_str()) ;
        madvise(m.data(), m.size(), MADV_RANDOM) ;
        const size_t msz = m.size() ;
        if (msz < head_size + tail_size)
            h.append_data(hash_bytes(m.data(), msz)) ;
        else
        {
            h.append_data(hash_bytes(m.data(), head_size)) ;
            h.append_data(hash_bytes(m.cdata() + head_size, tail_size)) ;
            //h.append_data(hash_bytes(m.cdata() + (msz - tail_size), tail_size)) ;
        }
        ++count ;
    }
    return {count, h} ;
}

unipair<size_t> run_multi_bench()
{
    typedef std::vector<uint64_t> csums ;
    typedef std::packaged_task<csums(const std::string *, const std::string *)> ctask ;

    auto calc = [](const std::string *b, const std::string *e) -> csums
    {
        csums cs ;
        for (; b != e ; ++b)
        {
            pcomn::PMemMapping m (b->c_str()) ;
            madvise(m.data(), m.size(), MADV_RANDOM) ;
            const size_t msz = m.size() ;
            if (msz < head_size + tail_size)
                cs.push_back(hash_bytes(m.data(), msz)) ;
            else
            {
                cs.push_back(hash_bytes(m.data(), head_size)) ;
                //cs.push_back(hash_bytes(m.cdata() + (msz - tail_size), tail_size)) ;
            }
        }
        return cs ;
    } ;

    std::vector<std::string> files (all_files()) ;
    pcomn::sort(files) ;

    std::vector<std::string> ff[4] ;
    size_t i = 0 ;
    for (std::string &s: files)
    {
        ff[i].emplace_back(std::move(s)) ;
        i = (i + 1) % std::size(ff) ;
    }

    const size_t count = files.size() ;

    ctask task[std::size(ff)] {ctask(calc), ctask(calc), ctask(calc), ctask(calc)} ;

    std::future<csums> rr[std::size(task)]
    {
        task[0].get_future(),
        task[1].get_future(),
        task[2].get_future(),
        task[3].get_future()
    } ;

    std::thread t1 (std::move(task[0]), ff[0].data(), ff[0].data() + ff[0].size()) ;
    std::thread t2 (std::move(task[1]), ff[1].data(), ff[1].data() + ff[1].size()) ;
    std::thread t3 (std::move(task[2]), ff[2].data(), ff[2].data() + ff[2].size()) ;
    std::thread t4 (std::move(task[3]), ff[3].data(), ff[3].data() + ff[3].size()) ;

    t1.detach() ;
    t2.detach() ;
    t3.detach() ;
    t4.detach() ;

    hash_combinator h ;
    for (std::future<csums> &r: rr)
        for (uint64_t s: r.get())
            h.append_data(s) ;

    return {count, h} ;
}

int main()
{
   try {
       //const unipair<size_t> cnt = run_bench() ;
       const unipair<size_t> cnt = run_multi_bench() ;
       std::cout << cnt.first << " regular files processed, checksum is " << HEXOUT(cnt.second) << std::endl ;
   }
   catch (const std::exception &x)
   {
      std::cerr << STDEXCEPTOUT(x) << std::endl ;
      return 1 ;
   }
}
