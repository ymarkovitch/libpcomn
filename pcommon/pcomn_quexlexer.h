/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __PCOMN_QUEXLEXER_H
#define __PCOMN_QUEXLEXER_H
/*******************************************************************************
 FILE         :   pcomn_quexlexer.h
 COPYRIGHT    :   Yakov Markovitch, 2010-2020. All rights reserved.

 DESCRIPTION  :   Wrapper class template for lexers generated by QueX

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   27 Sep 2010
*******************************************************************************/
#include <pcomn_binstream.h>
#include <pcomn_mmap.h>
#include <pcomn_strslice.h>
#include <pcomn_fstream.h>

namespace pcomn {
/// @cond
namespace detail {
/******************************************************************************/
/** @internal
*******************************************************************************/
struct fullstream_reader {
    fullstream_reader() : _stream_size(0), _b(0) {}

    fullstream_reader(binary_istream &in) :
        _stream_size(0)
    {
        _b = 0 ;

        size_t sz = 256 ;
        for (char *start = NULL ; !in.eof() ;)
        {
            if (!start || _stream_size == sz - 2)
            {
                start = (char *)ensure_allocated(realloc(_owned_buf.get(), sz <<= 1)) ;
                _owned_buf.release() ;
                _owned_buf.reset(start) ;
                ++start ;
            }
            _stream_size += in.read(start + _stream_size, sz - 2 - _stream_size) ;
            PCOMN_VERIFY(_stream_size < sz - 1) ;
        }
    }
    size_t              _stream_size ;
    malloc_ptr<char[]>  _owned_buf ;
    union {
        uint8_t     _zerobuf[4] ;
        uint32_t    _b ;
    } ;
} ;
} // end of namespace pcomn::detail
/// @endcond

/******************************************************************************/
/** Wrapper class template for lexers generated by QueX
*******************************************************************************/
template<class Lexer>
class QuexLexer : private detail::fullstream_reader, public Lexer {
    typedef Lexer ancestor ;
public:
    // FIXME: There must be Lexer-dependent type
    typedef uint8_t char_type ;

    explicit QuexLexer(const strslice &text, const char *encoding_name = NULL) ;

    explicit QuexLexer(binary_istream &in, const char *encoding_name = NULL) ;

    /// Create a lexer on a file.
    /// @note The lexer doesn't the own @a file
    explicit QuexLexer(FILE *file, const char *encoding_name = NULL) :
        ancestor(file, encoding_name)
    {}

    /// The current start of input
    const char *input_begin() const
    {
        return (char *)const_cast<QuexLexer *>(this)->buffer_lexeme_start_pointer_get() ;
    }
    /// The end of input
    const char *input_end() const
    {
        return (char *)const_cast<QuexLexer *>(this)->buffer_fill_region_begin() ;
    }

    /// Reset lexer line/column counter to line @a linenum column @a colnum.
    void set_line(unsigned linenum, unsigned colnum = 1)
    {
        set_qlexer_line(*this, linenum, colnum) ;
    }
} ;

/*******************************************************************************
 QuexLexer
*******************************************************************************/
template<class Lexer>
QuexLexer<Lexer>::QuexLexer(const strslice &text, const char *encoding_name) :
    ancestor(text.empty() ? _zerobuf : (char_type *)NULL,
             text.size() + 3 ,
             text.empty() ? _zerobuf + 1 : (char_type *)NULL,
             encoding_name)
{
    if (!text.empty())
        this->buffer_fill_region_append((char_type *)text.begin(), (char_type *)text.end()) ;
}

template<class Lexer>
QuexLexer<Lexer>::QuexLexer(binary_istream &in, const char *encoding_name) :
    detail::fullstream_reader(in),
    ancestor(_stream_size ? (char_type *)_owned_buf.get() : _zerobuf,
             _stream_size + 2,
             (_stream_size ? (char_type *)_owned_buf.get() : _zerobuf) + _stream_size + 1,
             encoding_name)
{}

/*******************************************************************************
 Global function
*******************************************************************************/
/// Reset QueX lexer line/column counter to line @a linenum column @a colnum.
template<class Lexer>
void set_qlexer_line(Lexer &qlexer, unsigned linenum, unsigned colnum = 1)
{
    qlexer.counter.base._line_number_at_begin = qlexer.counter.base._line_number_at_end = linenum ;
    qlexer.counter.base._column_number_at_begin = qlexer.counter.base._column_number_at_end = colnum ;
}

} // end of namespace pcomn

#endif /* __PCOMN_QUEXLEXER_H */
