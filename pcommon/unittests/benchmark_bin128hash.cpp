/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#include <pcomn_stopwatch.h>
#include <pcomn_hash.h>
#include <iostream>
#include <algorithm>

using namespace pcomn ;

static constexpr size_t SIDE = 2048 ;

static binary128_t v1[SIDE] ;
static binary128_t v2[SIDE] ;

__noinline binary128_t clock_hashing(size_t rounds)
{
    if (!rounds)
        return {} ;

    const size_t itemcount = SIDE*rounds ;
    std::cout << "Running " << rounds << " rounds (" << itemcount << " items)" << std::endl ;

    PCpuStopwatch  cpu_stopwatch ;
    PRealStopwatch wall_stopwatch ;

    binary128_t *from = v1 ;
    binary128_t *to = v2 ;

    wall_stopwatch.start() ;
    cpu_stopwatch.start() ;

    for (size_t n = 0 ; ++n < rounds ; std::swap(from, to))
    {
        std::transform(from, from + SIDE, to,
                       [](const binary128_t &v) { const uint64_t h = v.hash() ; return binary128_t(h, h) ; }) ;
    }

    cpu_stopwatch.stop() ;
    wall_stopwatch.stop() ;

    std::cout
        << cpu_stopwatch.elapsed() <<  "s CPU time,  " << cpu_stopwatch.elapsed()/itemcount  << "s per item, "
        << wall_stopwatch.elapsed() << "s real time, " << wall_stopwatch.elapsed()/itemcount << "s per item" << std::endl ;

    return *std::min_element(from, from + SIDE) ;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        return 1 ;
    int rounds = atoi(argv[1]) ;
    if (rounds <= 0)
        return 1 ;

    std::generate(std::begin(v1), std::end(v1), [init=0]() mutable { ++init ; return binary128_t(init, init) ; }) ;
    std::transform(std::begin(v1), std::end(v1), std::begin(v2),
                   [](const binary128_t &v) { const uint64_t h = v.hash() ; return binary128_t(h, h) ; }) ;

    const binary128_t minval = clock_hashing(rounds) ;

    std::cout << minval << std::endl ;

    return 0 ;
}
