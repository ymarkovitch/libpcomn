/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_makejrn.cpp
 COPYRIGHT    :   Yakov Markovitch, 2010-2014. All rights reserved.

 DESCRIPTION  :   Command-line utility for creating journal checkpoints from data read
                  from stdin

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   5 Apr 2010
*******************************************************************************/
#include <pcomn_journmmap.h>
#include <pcomn_getopt.h>
#include <pcomn_version.h>
#include <pcomn_fstream.h>
#include <pcomn_buffer.h>

#include <iostream>
#include <stdlib.h>

namespace pj = pcomn::jrn ;

// Argument map
static const char short_options[] = "" ;
static const struct option long_options[] = {
   // --help, --version
   PCOMN_DEF_STDOPTS()
} ;

static inline void print_version()
{
   printf("Create a journal checkpoint from stdin (%s)\n\n", PCOMN_BUILD_STRING) ;
}

static inline void print_usage()
{
   print_version() ;
   printf("Usage: %1$s JOURNAL_PATH\n"
          "       %1$s [--help|--version]\n"
          "\n"
          "Create a checkpoint from stdin\n"
          "\n"
          "Options:\n"
          "  --help                 display this help and exit\n"
          "  --version              output version information and exit\n\n"
          , PCOMN_PROGRAM_SHORTNAME) ;
}

class FakeJournallable : public pj::Journallable {
      typedef pj::Journallable ancestor ;
   public:
      explicit FakeJournallable(const pcomn::shared_buffer &buffer) :
         _buffer(buffer)
      {}
   protected:
      pj::operation_ptr create_operation(pj::opcode_t, pj::opversion_t) const
      {
         return pj::operation_ptr() ;
      }

      void restore_checkpoint(pcomn::binary_ibufstream &, size_t) {}
      void start_checkpoint() {}
      void finish_checkpoint() throw() {}

      void save_checkpoint(pcomn::binary_obufstream &checkpoint)
      {
         checkpoint.write(_buffer.get(), _buffer.size()) ;
      }

      bool fill_user_magic(pj::magic_t &magic) const
      {
         static const pj::magic_t JOURNAL_MAGIC = {{ '@', 'D', 'S', 't', 'r', 'e', 'e', '\0' }} ;
         magic = JOURNAL_MAGIC ;
         return true ;
      }

   private:
      const pcomn::shared_buffer _buffer ;
} ;

int main(int argc, char *argv[])
{
   for (int lastopt ; (lastopt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1 ;)
      switch(lastopt)
      {
         PCOMN_HANDLE_STDOPTS() ;
      }
   pcomn::cli::check_remaining_argcount(argc, optind, pcomn::cli::REQUIRED_ARGUMENT, 1, 1) ;

   try {
      pcomn::binary_ifdstream input (STDIN_FILENO, false) ;
      pcomn::basic_buffer ibuf ;
      char buf[1024] ;
      size_t sz = 0 ;

      for (size_t readcount ; (readcount = input.read(buf)) != 0 ; sz += readcount)
         memcpy(pcomn::padd(ibuf.grow(sz + readcount), sz), buf, readcount) ;

      FakeJournallable journallable (pcomn::shared_buffer(ibuf.data(), sz)) ;
      pcomn::PTSafePtr<pj::Port> journal
         (new pj::Port(new pj::MMapStorage(argv[optind], pj::MD_WRONLY))) ;
      journallable.set_journal(journal) ;
   }
   catch (const std::exception &x)
   {
      std::cerr << STDEXCEPTOUT(x) << std::endl ;
      return 1 ;
   }

   return 0 ;
}
