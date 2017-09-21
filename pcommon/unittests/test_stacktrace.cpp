/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#include <pcomn_regex.h>
#include <pcomn_stacktrace.h>
#include <pcomn_omanip.h>
#include <iostream>

using namespace pcomn ;
const size_t inbuf_size = 16*KiB ;

stack_trace test_rx_stacktrace (0) ;
stack_trace read_rx_stacktrace (0) ;
stack_trace test_all_stacktrace (0) ;
stack_trace main_stacktrace (0) ;

static std::ostream &print_stack(std::ostream &os, const stack_trace &bt)
{
    return os << "{size=" << bt.size() << ", tid=" << bt.thread_id() << '}' ;
}

void test_rx(const regex &exp, const char *str)
{
    reg_match sub[36] ;
    for (reg_match *first = sub, *last = exp.match(str, sub) ; first != last ; ++first)
        std::cout << strslice(str, *first) << std::endl ;

    test_rx_stacktrace = stack_trace() ;
}

const char *read_rx(char *str)
{
    read_rx_stacktrace = stack_trace() ;
    return std::cin.getline(str, inbuf_size) ? str : "" ;
}

inline void test_all(const char *rx)
{
    char inbuf[inbuf_size] ;
    const regex exp (rx) ;
    read_rx(inbuf) ;
    test_rx(exp, inbuf) ;

    test_all_stacktrace = stack_trace() ;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: " << *argv << " <regexp>" << std::endl ;
        return 255 ;
    }

    main_stacktrace = stack_trace() ;
    try
    {
        test_all(argv[1]) ;
    }
    catch (const pcomn::regex_error &err)
    {
        std::cout << err.what() << " in expression \"" << err.expression() <<
            "\" at position " << err.position() << std::endl ;
    }

    print_stack(std::cout << "test_rx ", test_rx_stacktrace) << std::endl ;
    print_stack(std::cout << "read_rx ", read_rx_stacktrace) << std::endl ;
    print_stack(std::cout << "test_all ", test_all_stacktrace) << std::endl ;
    print_stack(std::cout << "main ", main_stacktrace) << std::endl ;
    {
        resolved_frame frames[32] ;
        resolved_frame * const end = test_rx_stacktrace.resolve(std::begin(frames), std::end(frames)) ;
        std::cout << pcomn::oseqdelim(std::begin(frames), end, '\n') << std::endl ;
    }

    return 0 ;
}
