/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_rawstreamvcl.cpp
 COPYRIGHT    :   Yakov Markovitch, 2002-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test of using TStream with raw_stream_wrapper

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   17 Oct 2002
*******************************************************************************/
#include <vcl.h>
#include <pcomn_rawstream.h>
#include <pcomn_unistd.h>
#include <pcomn_safeptr.h>

#include <stdio.h>

namespace pcomn {

static raw_ios::pos_type seek_tstream(TStream *stream, raw_ios::off_type off, raw_ios::seekdir dir)
{
   Word origin ;
   switch (dir)
   {
      case raw_ios::cur:
         origin = soFromCurrent ; break ;
      case raw_ios::beg:
         origin = soFromBeginning ;
         if (!off)
            return stream->Position ;
         break ;
      case raw_ios::end:
         origin = soFromEnd ; break ;

      default: return raw_ios::pos_type(-1) ;
   }
   int position = stream->Seek(off, origin) ;
   return raw_ios::pos_type(position < 0 ? -1 : position) ;
}

template<class RawStream>
class raw_vclstream : public raw_stream_wrapper<TStream, RawStream> {
   protected:
      typedef raw_stream_wrapper<TStream, RawStream> Parent ;

      explicit raw_vclstream(TStream *stream = NULL, bool owns = false) :
         Parent(stream, owns)
      {}

      raw_ios::pos_type seekoff(raw_ios::off_type, raw_ios::seekdir) ;
} ;

template<class RawStream>
raw_ios::pos_type raw_vclstream<RawStream>::seekoff(raw_ios::off_type off, raw_ios::seekdir dir)
{
   return seek_tstream(&stream(), off, dir) ;
}

/*******************************************************************************
                     class raw_basic_istream
*******************************************************************************/
class raw_vclistream : public raw_vclstream<raw_istream> {
   public:
      typedef raw_vclstream<raw_istream> Parent ;

      explicit raw_vclistream(TStream *stream = NULL, bool owns = false) :
         Parent(stream, owns)
      {}

   protected:
      size_t do_read(void *buffer, size_t size) ;
} ;

/*******************************************************************************
                     class raw_basic_ostream
*******************************************************************************/
class raw_vclostream : public raw_vclstream<raw_ostream> {
   public:
      typedef raw_vclstream<raw_ostream> Parent ;

      explicit raw_vclostream(TStream *stream = NULL, bool owns = false) :
         Parent(stream, owns)
      {}

   protected:
      size_t do_write(const void *buffer, size_t size) ;
} ;

/*******************************************************************************
 raw_basic_istream<C, T>
*******************************************************************************/
size_t raw_vclistream::do_read(void *buffer, size_t size)
{
   return stream().Read(buffer, size) ;
}

/*******************************************************************************
 raw_basic_ostream<C, T>
*******************************************************************************/
size_t raw_vclostream::do_write(const void *buffer, size_t size)
{
   stream().WriteBuffer(buffer, size) ;
   return size ;
}

}

static const int DWIDTH = 6 ;

static void writeseq(pcomn::raw_ostream &stream, int from, int to)
{
   std::cout << "Writing from " << from << " to " << to << " through pcomn::raw_ostream" << std::endl ;
   AnsiString str ;
   while(from < to)
   {
      str.sprintf("%*d", DWIDTH, from++) ;
      stream.write(str.c_str(), str.Length()) ;
   }
   std::cout << "OK" << std::endl ;
}

static void writeseq(TStream &stream, int from, int to)
{
   std::cout << "Writing from " << from << " to " << to << " through TStream" << std::endl ;
   AnsiString str ;
   while(from < to)
   {
      str.sprintf("%*d", DWIDTH, from++) ;
      stream.Write(str.c_str(), str.Length()) ;
   }
   std::cout << "OK" << std::endl ;
}

static void check_val(const char *value, int standard)
{
   if (AnsiString(value).ToInt() != standard)
      throw std::runtime_error(("Difference: standard=" + AnsiString(standard) +
                                " file=" + value).c_str()) ;
}

static void readseq(pcomn::raw_istream &stream, int from, int to)
{
   std::cout << "Reading from " << from << " to " << to << " through pcomn::raw_istream" << std::endl ;
   for(char buf[DWIDTH + 1] = "\0\0\0\0\0\0" ; from < to ; ++from)
      if (!stream.read(buf, sizeof buf - 1))
         if (stream.eof())
         {
            std::cout << "EOF reading from a pcomn::raw_istream" << std::endl ;
            break ;
         }
         else
         {
            std::cout << (stream.bad() ? "Error " : "Failure ")
                   << "reading from a pcomn::raw_istream" << std::endl ;
            break ;
         }
      else
         check_val(buf, from) ;
   std::cout << "OK" << std::endl ;
}

static void readseq(TStream &stream, int from, int to)
{
   std::cout << "Reading from " << from << " to " << to << " through TStream" << std::endl ;
   for(char buf[7] = "\0\0\0\0\0\0" ; from < to ; ++from)
   {
      stream.Read(buf, sizeof buf - 1) ;
      check_val(buf, from) ;
   }
   std::cout << "OK" << std::endl ;
}

int main()
{
   AnsiString name (PTMallocPtr<char>(tempnam(NULL, "~ptst"))) ;
   int result = 1 ;
   try {
      try {
         AnsiString str ;
         std::cout << "Creating raw_ostream" << std::endl ;
         pcomn::raw_vclostream os (new TFileStream(name, fmCreate), true) ;

         writeseq(os, 0, 2000) ;
         std::cout << "tellp=" << os.tellp() << std::endl ;
         writeseq(os.stream(), 2000, 6000) ;
         writeseq(os, 6000, 6001) ;
         std::cout << "tellp=" << os.tellp() << std::endl ;
         writeseq(os.stream(), 6001, 6002) ;
         writeseq(os.stream(), 6002, 10000) ;
         std::cout << "tellp=" << os.tellp() << std::endl ;
         os.close() ;

         std::cout << "Creating raw_istream" << std::endl ;
         pcomn::raw_vclistream is (new TFileStream(name, fmOpenRead), true) ;
         std::cout << "tellg=" << is.tellg() << std::endl ;

         readseq(is.stream(), 0, 1) ;
         readseq(is, 1, 100) ;
         is.seekg(DWIDTH * 200) ;
         std::cout << "tellg=" << is.tellg() << std::endl ;
         readseq(is.stream(), 200, 4000) ;
         std::cout << "tellg=" << is.tellg() << std::endl ;
         is.seekg(-DWIDTH * 1000, pcomn::raw_ios::cur) ;
         std::cout << "tellg=" << is.tellg() << std::endl ;
         readseq(is, 3000, 10000) ;
         // Testing the end-of-file condition
         readseq(is, 10000, 20000) ;
         result = 0 ;
      }
      catch(const std::exception &x)
      {
         std::cout << x.what() << std::endl ;
         throw ;
      }
      catch(Exception &e)
      {
         std::cout << e.Message.c_str() << std::endl ;
         throw ;
      }
   }
   catch(...)
   {}
   if (!access(name.c_str(), 0))
      unlink(name.c_str()) ;
   return result ;
}
