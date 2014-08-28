/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_rawstream.cpp
 COPYRIGHT    :   Yakov Markovitch, 2002-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test for raw streams.
                  This test creates raw stream wrappers for usual std::streams.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   12 Oct 2002
*******************************************************************************/
#include <pcomn_rawstream.h>
#include <pcomn_string.h>
#include <pcomn_safeptr.h>
#include <stdio.h>
#include <typeinfo>

#include <fstream>

static const int DWIDTH = 6 ;

static void writeseq(std::ostream &os, int from, int to)
{
   std::cout << "Writing from " << from << " to " << to << " through ostream" << std::endl ;
   while(from < to)
      os << pcomn::strprintf("%*d", DWIDTH, from++) ;
   std::cout << "OK" << std::endl ;
}

static void writeseq(pcomn::raw_ostream &stream, int from, int to)
{
   std::cout << "Writing from " << from << " to " << to << " through pcomn::raw_stream" << std::endl ;
   while(from < to)
   {
      std::string str (pcomn::strprintf("%*d", DWIDTH, from++)) ;
      stream.write(str.c_str(), str.size()) ;
   }
   std::cout << "OK" << std::endl ;
}

static void check_val(const char *value, int standard)
{
   if (atoi(value) != standard)
      throw std::runtime_error(pcomn::strprintf("Difference: standard=%d; file=%s", standard, value)) ;
}

template<class Stream>
void read_seq(Stream &stream, int from, int to)
{
   std::cout << "Reading from " << from << " to " << to << " through " << PCOMN_TYPENAME(stream) << std::endl ;
   for(char buf[DWIDTH + 1] = "\0\0\0\0\0\0" ; from < to ; ++from)
      if (!stream.read(buf, sizeof buf - 1))
         if (stream.eof())
         {
            std::cout << "EOF reading from a pcomn::raw_istream" << std::endl ;
            return ;
         }
         else
         {
            std::cout << (stream.bad() ? "Error " : "Failure ")
                   << "reading from a pcomn::raw_istream" << std::endl ;
            return ;
         }
      else
         check_val(buf, from) ;

   std::cout << "OK" << std::endl ;
}

inline void readseq(pcomn::raw_istream &stream, int from, int to)
{
   read_seq(stream, from , to) ;
}

inline void readseq(std::istream &stream, int from, int to)
{
   read_seq(stream, from , to) ;
}

static void test_exceptions(pcomn::raw_istream &stream)
{
   std::cout << std::endl ;
   try {
      readseq(stream, 0, 9999) ;
      char buf[1024] ;
      stream.read(buf, sizeof buf) ;
   }
   catch (const std::exception &x)
   {
      std::cout << "Exception: " << x.what() << std::endl ;
   }
   std::cout << stream.last_read() << " bytes read by the last operation" << std::endl ;
   (stream.rdstate() & pcomn::raw_ios::eofbit) && (std::cout << "eofbit set" << std::endl) ;
   (stream.rdstate() & pcomn::raw_ios::failbit) && (std::cout << "failbit set" << std::endl) ;
   (stream.rdstate() & pcomn::raw_ios::badbit) && (std::cout << "badbit set" << std::endl) ;
}

int main()
{
   malloc_ptr<char[]> name(tempnam(NULL, "~ptst")) ;
   int result = 1 ;
   try {
      try {
         std::cout << "Creating raw_ostream" << std::endl ;
         pcomn::raw_stdostream os (new std::ofstream(name.c_str()), true) ;

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
         pcomn::raw_stdistream is (new std::ifstream(name), true) ;
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
         std::cout << "Unexpected exception " << PCOMN_TYPENAME(x) << ": " << x.what() << std::endl ;
         throw ;
      }
      pcomn::raw_stdistream is ;

      is.owns(true) ;
      is.open(new std::ifstream(name)) ;
      is.exceptions(pcomn::raw_ios::eofbit) ;
      test_exceptions(is) ;

      is.open(new std::ifstream(name)) ;
      is.exceptions(pcomn::raw_ios::failbit|pcomn::raw_ios::eofbit) ;
      test_exceptions(is) ;

      is.open(new std::ifstream(name)) ;
      is.exceptions(pcomn::raw_ios::badbit) ;
      is.stream().exceptions(std::ios_base::failbit) ;
      test_exceptions(is) ;

      is.open(new std::ifstream(name)) ;
      is.exceptions(pcomn::raw_ios::goodbit) ;
      is.stream().exceptions(std::ios_base::failbit) ;
      test_exceptions(is) ;
      is.close() ;
   }
   catch(...)
   {}
   if (!access(name, 0))
      unlink(name) ;
   return result ;
}
