/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_mkjournal.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Make journal test

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   28 Nov 2008
*******************************************************************************/
#include "test_journal.h"
#include <pcomn_journmmap.h>
#include <pcomn_getopt.h>
#include <pcomn_version.h>
#include <pcomn_string.h>
#include <pcomn_strnum.h>

#include <fstream>
#include <stdlib.h>

using namespace pcomn::jrn ;

// Argument map
static const char short_options[] = "s:i:n" ;
static const struct option long_options[] = {
   {"segdir",        1, NULL, 's'},
   {"input",         1, NULL, 'i'},
   {"only-storage",  1, NULL, 'n'},

   // --help, --version
   PCOMN_DEF_STDOPTS()
} ;

static inline void print_version()
{
   printf("PCOMMON make journal test (%s)\n\n", PCOMN_BUILD_STRING) ;
}

static inline void print_usage()
{
   print_version() ;
   printf("Usage: %1$s [OPTIONS] JOURNAL_PATH\n"
          "       %1$s [--help|--version]\n"
          "\n"
          "Create a writeahead journal for a mockup journallable string map.\n"
          "\n"
          "Options:\n"
          "  -i [--input]  ARG   specify a file with initial data; '-i -' means stdin\n"
          "  -n [--only-storage] don't create a journallable object, create only storage\n"
          "  -s [--segdir] ARG   specify a directory for journal segments\n"
          "  --help              display this help and exit\n"
          "  --version           output version information and exit\n\n"
          , PCOMN_PROGRAM_SHORTNAME) ;
}

static JournallableStringMap *map_from_file(const char *filename)
{
   std::cout << "Reading initial test data from '" << filename << "'... " ;

   std::ifstream input (filename) ;
   PCOMN_THROW_IF(!input, std::runtime_error, "Cannot open '%s'", filename) ;

   JournallableStringMap * const result =
      JournallableStringMap::from_stream(input) ;

   std::cout << "OK " << result->size() << " items." ;

   return result ;
}

/*******************************************************************************

********************************************************************************/
int main(int argc, char *argv[])
{
   DIAG_INITTRACE("test_mkjournal.trace.ini") ;

   const char *segdir = NULL ;
   const char *input = NULL ;
   bool only_storage = false ;

   for (int lastopt ; (lastopt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1 ;)
      switch(lastopt)
      {
         PCOMN_SETOPT_VAL('s', segdir) ;
         PCOMN_SETOPT_VAL('i', input) ;
         PCOMN_SETOPT_FLAG('n', only_storage) ;
         PCOMN_HANDLE_STDOPTS() ;
      }
   pcomn::cli::check_remaining_argcount(argc, optind, pcomn::cli::REQUIRED_ARGUMENT, 1, 1) ;

   try {
      if (only_storage)
      {
         MMapStorage storage (argv[optind], segdir) ;
      }
      else if (!input)
      {
         pcomn::PTSafePtr<JournallableStringMap> data (new JournallableStringMap) ;

         std::cout << "*** Opening an existent journal ***" << std::endl ;

         Port journal (new MMapStorage(argv[optind])) ;

         std::cout << "*** Replaying the journal  ***" << std::endl ;
         data->restore_from(journal, false) ;
      }
      else
      {
         pcomn::PTSafePtr<JournallableStringMap> data (map_from_file(input)) ;

         std::cout << "*** Creating a new journal ***" << std::endl ;

         Port journal (new MMapStorage(argv[optind], segdir)) ;

         std::cout << "*** Connecting the data object to the actual journal and taking checkpoint ***" << std::endl ;
         data->set_journal(&journal) ;
      }

   } catch(const std::exception& e) {
      std::cerr << "\nError: " << e.what() << std::endl ;
      return EXIT_FAILURE ;
   }
   return EXIT_SUCCESS ;
}
