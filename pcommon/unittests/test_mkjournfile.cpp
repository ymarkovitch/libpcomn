/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_mkjournfile.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Journal files test

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   12 Dec 2008
*******************************************************************************/
#include "test_journal.h"
#include <pcomn_journmmap.h>
#include <pcomn_journstorage.h>
#include <pcomn_getopt.h>
#include <pcomn_version.h>
#include <pcomn_sys.h>

#include <fstream>
#include <stdlib.h>

using namespace pcomn::jrn ;

// Argument map
static const char short_options[] = "icg:m:t:" ;
static const struct option long_options[] = {
   {"init",        0, NULL, 'i'},
   {"commit",      0, NULL, 'c'},
   {"generation",  1, NULL, 'g'},
   {"user-magic",  1, NULL, 'm'},
   {"type",        1, NULL, 't'},

   // --help, --version
   PCOMN_DEF_STDOPTS()
} ;

static inline void print_version()
{
   printf("PCOMMON make journal file test (%s)\n\n", PCOMN_BUILD_STRING) ;
}

static inline void print_usage()
{
   print_version() ;
   printf("Usage: %1$s [OPTIONS] JOURNAL_PATH\n"
          "       %1$s [--help|--version]\n"
          "\n"
          "Create a checkpoint of segment file.\n"
          "\n"
          "Options:\n"
          "  -c [--commit]          commit the created file\n"
          "  -i [--init]            init the created file\n"
          "  -g [--generation] ARG  specify a generation (default is 0)\n"
          "  -m [--user-magic] ARG  specify a user magic number (at most 8 characters)\n"
          "  -t [--type] ARG        specify file type ('seg' or 'cp'). Mandatory option.\n"
          "  --help                 display this help and exit\n"
          "  --version              output version information and exit\n\n"
          , PCOMN_PROGRAM_SHORTNAME) ;
}

bool perform_commit = false ;
bool perform_init = false ;
magic_t user_magic = JournallableStringMap::MAGIC ;

static void test_checkpoint(int dirfd, const std::string &journal_name, generation_t generation)
{
   pj::MMapStorage::CheckpointFile cpf (dirfd, journal_name, generation, 0600) ;

   if (perform_init)
      cpf.init(user_magic) ;
   if (perform_commit)
      cpf.commit() ;
}

static void test_segment(int dirfd, const std::string &journal_name, generation_t generation)
{
   pj::MMapStorage::SegmentFile segf (dirfd, journal_name, generation, 0600) ;

   if (perform_init)
      segf.init(user_magic) ;
   if (perform_commit)
      segf.commit(0) ;
}

/*******************************************************************************

********************************************************************************/
int main(int argc, char *argv[])
{
   DIAG_INITTRACE("test_mkjournfile.trace.ini") ;

   const char *generation_option = NULL ;
   const char *user_magic_option = NULL ;
   const char *type_option = NULL ;

   for (int lastopt ; (lastopt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1 ;)
      switch(lastopt)
      {
         PCOMN_SETOPT_VAL('g', generation_option) ;
         PCOMN_SETOPT_VAL('m', user_magic_option) ;
         PCOMN_SETOPT_VAL('t', type_option) ;
         PCOMN_SETOPT_FLAG('c', perform_commit) ;
         PCOMN_SETOPT_FLAG('i', perform_init) ;
         PCOMN_HANDLE_STDOPTS() ;
      }
   pcomn::cli::check_remaining_argcount(argc, optind, pcomn::cli::REQUIRED_ARGUMENT, 1, 1) ;

   if (!type_option)
      pcomn::cli::exit_invalid_arg("'-t' ('--type') option is mandatory.") ;

   const bool is_checkpoint = !strcmp(type_option, "cp") ;

   if (!is_checkpoint && strcmp(type_option, "seg"))
      pcomn::cli::exit_invalid_arg("Type option should be 'cp' or 'seg'.") ;

   if (user_magic_option)
   {
      if (strlen(user_magic_option) > sizeof user_magic)
         pcomn::cli::exit_invalid_arg("User magic is too long.") ;
      strncpy(user_magic.data, user_magic_option, sizeof user_magic) ;
   }

   const char *endptr = "" ;
   const pcomn::ulonglong_t generation =
      generation_option ? strtoll(generation_option, (char **)&endptr, 10) : 0 ;

   if (*endptr)
      pcomn::cli::exit_invalid_arg("Invalid generation.") ;

   // Check generation and user magic
   try {
      const std::string journal_dir = MMapStorage::journal_dir_from_path(argv[optind]) ;
      const char *journal_name = MMapStorage::journal_name_from_path(argv[optind]) ;
      const std::string filename =
         MMapStorage::build_filename(is_checkpoint ? MMapStorage::NK_CHECKPOINT : MMapStorage::NK_SEGMENT,
                                     journal_name, generation) ;

      std::cout << "*** Creating a new journal " << (is_checkpoint ? "CHECKPOINT" : "SEGMENT")
                << " file '" << journal_name << "' at '" << journal_dir << "'" << std::endl ;

      const pcomn::fd_safehandle dirfd (pcomn::sys::opendir(journal_dir.c_str())) ;

      if (is_checkpoint)
         test_checkpoint(dirfd, filename, generation) ;
      else
         test_segment(dirfd, filename, generation) ;

   } catch(const std::exception& e) {
      std::cerr << "\n" << STDEXCEPTOUT(e) << std::endl ;
      return EXIT_FAILURE ;
   }
   return EXIT_SUCCESS ;
}
