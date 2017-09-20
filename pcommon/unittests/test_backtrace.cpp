/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#include <pcomn_stacktrace.h>
#include <pcomn_regex.h>

#include <iostream>

using namespace pcomn ;

void test_rx(const regex &exp, const char *str)
{
    reg_match sub[36] ;
    for (reg_match *first = sub, *last = exp.match(str, sub) ; first != last ; ++first)
        std::cout << strslice(str, *first) << std::endl ;
}

const char *read_rx(char *str)
{
    return std::cin.getline(str, 1024*1024) ? str : nullptr ;
}

static struct {
    char inbuf[64] ;
    char *buf = inbuf ;
} data ;

int main(int argc, char *argv[])
{
    enable_dump_on_abend() ;

    if (argc != 2)
    {
        std::cout << "Usage: " << *argv << " <regexp>" << std::endl ;
        return 255 ;
    }

    const regex exp (argv[1]) ;
    read_rx(data.inbuf) ;
    test_rx(exp, data.buf) ;
    return 0 ;
}
