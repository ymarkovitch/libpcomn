/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pjourninfo.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   PCommon journal info command-line utility

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   24 Nov 2008
*******************************************************************************/
#include <pcomn_journmmap.h>
#include <pcomn_getopt.h>
#include <pcomn_version.h>
#include <pcomn_string.h>
#include <pcomn_strnum.h>
#include <pcomn_binascii.h>

#include <iostream>
#include <stdlib.h>

using namespace pcomn::jrn ;

// Argument map
static const char short_options[] = "" ;
static const struct option long_options[] = {
   // --help, --version
   PCOMN_DEF_STDOPTS()
} ;

static inline void print_version()
{
   printf("PCOMMON journal info utility (%s)\n\n", PCOMN_BUILD_STRING) ;
}

static inline void print_usage()
{
   print_version() ;
   printf("Usage: %1$s <subcommand> [OPTIONS] [ARGS]\n"
          "       %1$s [--help|--version]\n"
          "Type '%1$s <subcommand> --help' for help on a specific subcommand.\n"
          "\n"
          "Available subcommands:\n"
          "  list (ls)\n"
          "  namecheck (nc)\n"
          "\n"
          "Global options:\n"
          "  --help           display this help and exit\n"
          "  --version        output version information and exit\n\n"
          , PCOMN_PROGRAM_SHORTNAME) ;
}

/*******************************************************************************

*******************************************************************************/
#define HANDLE_NAMEKINDS()                      \
   NAMEKIND_APPLY(SEGDIR)                       \
   NAMEKIND_APPLY(SEGMENT)                      \
   NAMEKIND_APPLY(CHECKPOINT)                   \
   NAMEKIND_APPLY(UNKNOWN)

static const char *namekind_to_name(MMapStorage::FilenameKind kind)
{
#define NAMEKIND_APPLY(nk) case MMapStorage::NK_##nk: return #nk ;
   switch(kind)
   {
      HANDLE_NAMEKINDS()
      default: return "INVALID" ;
   }
#undef NAMEKIND_APPLY
}

static MMapStorage::FilenameKind name_to_kind(const char *name)
{
#define NAMEKIND_APPLY(nk) if (!strcmp(name, #nk)) return MMapStorage::NK_##nk ;
   HANDLE_NAMEKINDS() ;
#undef NAMEKIND_APPLY
   return MMapStorage::NK_UNKNOWN ;
}

static const char *filekind_to_name(MMapStorage::FileKind kind)
{
   return kind == MMapStorage::KIND_SEGMENT
      ? "SEGMENT"
      : (kind == MMapStorage::KIND_CHECKPOINT ?  "CHECKPOINT" : "UNKNOWN") ;
}

static MMapStorage::FileKind name_to_filekind(const char *name)
{
   if (!strcmp(name, "SEGMENT")) return MMapStorage::KIND_SEGMENT ;
   if (!strcmp(name, "CHECKPOINT")) return MMapStorage::KIND_CHECKPOINT ;
   return MMapStorage::KIND_UNKNOWN ;
}

/*******************************************************************************
 subcommand_list
*******************************************************************************/
struct subcommand_list {
      static const char short_options[] ;
      static const struct option long_options[] ;

      typedef std::vector<std::string> namevector_type ;

      static inline void print_usage()
      {
         puts("list (ls): list journals in a directory and/or journal files for a journal.\n"

              "usage: list [OPTIONS] PATH\n"
              "\n"
              "OPTIONS:\n"
              "  -a, --all             list all journals in the PATH directory. This option\n"
              "                        implies PATH is a directory; without this option, PATH\n"
              "                        is a journal path and the utility lists components of\n"
              "                        the journal\n"
              "  -c, --check           check format of the journal component(s) and ignore journal\n"
              "                        files of unknown format. If -a specified, ignore journals\n"
              "                        that have no properly closed checkpoints.\n"
              "                        Without this option only file names are checked.\n"
              "  -k, --kind=KIND       list only journal components of KIND, where KIND is\n"
              "                        SEGMENT or CHECKPOINT\n"
              "  -l, --long            use long listing format\n"
              "\n"
              "  -0, --null            terminate output lines by a null character instead of endline\n") ;
      }

      static void exec(int argc, char *argv[])
      {
         printf("subcommand_list\n") ;
         bool null_endl = false ;
         const char *kind_to_list = filekind_to_name(MMapStorage::KIND_UNKNOWN) ;

         for (int lastopt ; (lastopt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1 ;)
            switch(lastopt)
            {
               PCOMN_SETOPT_FLAG('a', _all) ;
               PCOMN_SETOPT_FLAG('c', _check) ;
               PCOMN_SETOPT_VAL('k', kind_to_list) ;
               PCOMN_SETOPT_FLAG('l', _long_format) ;
               PCOMN_SETOPT_FLAG('0', null_endl) ;
               PCOMN_HANDLE_STDOPTS() ;
            }
         if (null_endl)
            _endl = '\0' ;
         _kind = name_to_filekind(kind_to_list) ;

         pcomn::cli::check_remaining_argcount(argc, optind, pcomn::cli::REQUIRED_ARGUMENT, 1) ;

         if (_all)
            list_all(argv[optind]) ;
         else
            list_journal(pcomn::str::cstr(MMapStorage::journal_dir_from_path(argv[optind])),
                         pcomn::str::cstr(MMapStorage::journal_name_from_path(argv[optind]))) ;
      }

      static void list_all(const char *dirname)
      {
      }

      static void list_journal(const char *dirname, const char *journal_name)
      {
         pcomn::dir_safehandle dir (::opendir(dirname)) ;

         PCOMN_THROW_IF(!dir, pcomn::system_error, "Cannot open directory '%s' for reading", dirname) ;

         if (_kind == MMapStorage::KIND_UNKNOWN || _kind == MMapStorage::KIND_CHECKPOINT)
            list_components(dir, journal_name, MMapStorage::KIND_CHECKPOINT) ;

         if (_kind == MMapStorage::KIND_UNKNOWN || _kind == MMapStorage::KIND_SEGMENT)
            list_components(dir, journal_name, MMapStorage::KIND_SEGMENT) ;
      }

      static void list_components(DIR *dir, const char *journal_name, MMapStorage::FileKind kind)
      {
         PCOMN_VERIFY(dir) ;
         PCOMN_VERIFY(journal_name) ;
         PCOMN_VERIFY(kind == MMapStorage::KIND_CHECKPOINT || kind == MMapStorage::KIND_SEGMENT) ;

         const pcomn::PTSafePtr<namevector_type> names
            (MMapStorage::list_files(dir, journal_name, kind)) ;
         NOXCHECK(names) ;

         list_components(dirfd(dir), &*names->begin(), &*names->end(), kind) ;
      }

      static void list_components(int dfd, const std::string *begin, const std::string *end,
                                  MMapStorage::FileKind kind)
      {
         while (begin != end)
            list_component(dfd, begin->c_str(), kind, _check, _long_format) ;
      }

      static MMapStorage::FileKind list_component(int dfd, const char *name,
                                                  MMapStorage::FileKind kind,
                                                  bool check, bool long_format)
      {
         if (!long_format && !check)
         {
            list_short(name) ;
            return kind ;
         }

         // Open the file. Ignore errors: take care of the case when the file
         // disappeared/renamed,
         const pcomn::fd_safehandle fd (::openat(dfd, name, O_RDONLY)) ;
         if (fd < 0)
            return MMapStorage::KIND_UNKNOWN ;

         const MMapStorage::FileStat &info = MMapStorage::file_stat(fd) ;

         // Skip the file if check failed
         if (!check || info.kind == kind)
            if (long_format)
               list_long(name, info) ;
            else
               list_short(name) ;

         return info.kind ;
      }

      static void list_short(const char *name)
      {
         printf("%s%c", name, _endl) ;
      }

      static void list_long(const char *name, const MMapStorage::FileStat &st)
      {
         printf("%c %19llu %19llu %9u %-16s %s%c",
                *filekind_to_name(st.kind),
                (pcomn::ulonglong_t)st.generation,
                (pcomn::ulonglong_t)st.datalength,
                (unsigned)st.opcount,
                pcomn::str::cstr(b2a_cstring(&st.user_magic, sizeof st.user_magic)),
                name, _endl) ;
      }

      static bool _all ;
      static bool _check ;
      static bool _long_format ;
      static char _endl ;
      static MMapStorage::FileKind _kind ;
} ;

bool subcommand_list::_all             = false ;
bool subcommand_list::_check           = false ;
bool subcommand_list::_long_format     = false ;
char subcommand_list::_endl            = '\n' ;
MMapStorage::FileKind subcommand_list::_kind = MMapStorage::KIND_UNKNOWN ;

const char subcommand_list::short_options[] = "0Iaslk:" ;
const struct option subcommand_list::long_options[] = {
   { "all",             0,  NULL,   'a' },
   { "check",           0,  NULL,   'c' },
   { "kind",            1,  NULL,   'k' },
   { "long",            0,  NULL,   'l' },
   { "null",            0,  NULL,   '0' },
   { "ignore-unknown",  0,  NULL,   'U' },
   // --help, --version
   PCOMN_DEF_STDOPTS()
} ;

/*******************************************************************************
 subcomand_namecheck
*******************************************************************************/
struct subcomand_namecheck {
      static const char short_options[] ;
      static const struct option long_options[] ;

      static inline void print_usage()
      {
         puts("namecheck (nc): Check or build the name of a journal or a journal component.\n"

              "usage: namecheck [OPTIONS] NAME\n"
              "       namecheck -b|--build=KIND NAME [GENERATION]\n"
              "\n"
              "  A journal component is a segment file, segment directory link, or a checkpoint\n"
              "  file.\n"
              "\n"
              "OPTIONS:\n"
              "  -c, --check       check whether the NAME is a valid journal name (DEFAULT)\n"
              "  -k, --get-kind    providing that NAME is a filename of a journal component,\n"
              "                    get the component kind, journal name, and generation\n"
              "  -b, --build=ARG   build a filename for a component of kind ARG for\n"
              "                    a journal NAME and generation GENERATION; if GENERATION\n"
              "                    omitted, use 0; if ARG omitted, build names for all kinds\n") ;
      }

      static void exec(int argc, char *argv[])
      {
         for (int lastopt ; (lastopt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1 ;)
            switch(lastopt)
            {
               case 'b': _build = "" ; break ;
               PCOMN_SETOPT_VAL('B', _build) ;
               PCOMN_SETOPT_FLAG('c', _check) ;
               PCOMN_SETOPT_FLAG('k', _get_kind) ;
               PCOMN_HANDLE_STDOPTS() ;
            }
         pcomn::cli::check_remaining_argcount(argc, optind, pcomn::cli::REQUIRED_ARGUMENT) ;

         const char * const name = argv[optind] ;
         const char * const generation = argv[optind + 1] ;

         if (_get_kind)
            get_kind(name) ;
         else if (_build)
            build(name, generation) ;
         else
            check(name) ;
      }

      static void check(const char *name)
      {
         const int status = !MMapStorage::is_valid_name(name) ;
         fprintf(stderr, status ? "INVALID\n" : "VALID\n") ;
         exit(status) ;
      }

      static void get_kind(const char *filename)
      {
         std::string journal_name ;
         generation_t generation ;

         const MMapStorage::FilenameKind kind =
            MMapStorage::parse_filename(filename, &journal_name, &generation) ;
         if (!kind)
            printf("%s\n", namekind_to_name(kind)) ;
         else if (generation != NOGEN)
            printf("%s %s %llu\n", namekind_to_name(kind), journal_name.c_str(), (pcomn::ulonglong_t)generation) ;
         else
            printf("%s %s\n", namekind_to_name(kind), journal_name.c_str()) ;
         exit(0) ;
      }

      static void build(const char *name, const char *generation_str)
      {
         using namespace pcomn ;

         if (!MMapStorage::is_valid_name(name))
         {
            fprintf(stderr, "Invalid journal name: '%s'\n", name) ;
            exit(1) ;
         }
         generation_t generation = 0 ;
         if (generation_str)
            pcomn::strtonum(generation_str, generation) ;

         if (!*_build)
            // Print all names
            printf("%s %s %s\n",
                   str::cstr(MMapStorage::build_filename(MMapStorage::NK_SEGDIR, name)),
                   str::cstr(MMapStorage::build_filename(MMapStorage::NK_SEGMENT, name, generation)),
                   str::cstr(MMapStorage::build_filename(MMapStorage::NK_CHECKPOINT, name, generation))) ;

         else if (MMapStorage::FilenameKind kind = name_to_kind(_build))
            printf("%s\n", str::cstr(MMapStorage::build_filename(kind, name, generation))) ;

         else
         {
            fprintf(stderr, "Invalid filename kind: '%s'\n", _build) ;
            exit(1) ;
         }

         exit(0) ;
      }

      static bool _check ;
      static bool _get_kind ;
      static const char *_build ;
} ;

bool subcomand_namecheck::_check ;
bool subcomand_namecheck::_get_kind ;
const char *subcomand_namecheck::_build ;

const char subcomand_namecheck::short_options[] = "cbk" ;
const struct option subcomand_namecheck::long_options[] = {
   { "check",       0,  NULL,   'c' },
   { "get-kind",    0,  NULL,   'k' },
   { "build",       1,  NULL,   'B' },
   // --help, --version
   PCOMN_DEF_STDOPTS()
} ;

/*******************************************************************************

*******************************************************************************/
typedef void (*subcommand)(int, char *[]) ;

static subcommand select_subcommand(int argc, char *argv[])
{
   if (!strcmp(argv[1], "ls") || !strcmp(argv[1], "list"))
      return &subcommand_list::exec ;

   if (!strcmp(argv[1], "nc") || !strcmp(argv[1], "namecheck"))
      return &subcomand_namecheck::exec ;

   pcomn::cli::exit_invalid_arg(PCOMN_BUFPRINTF(256, "Unknown subcommand: '%s'", argv[1])) ;

   return NULL ;
}

int main(int argc, char *argv[])
{
   DIAG_INITTRACE("pjourninfo.trace.ini") ;

   // Handle the case when the first argument is not a subcommand but --help/--version
   PCOMN_CHECK_SUBCOMMAND_ARG(argc, argv) ;

   subcommand command = select_subcommand(argc, argv) ;
   try {
      getopt_reset() ;
      command(--argc, ++argv) ;

   } catch(const std::exception& e) {
      std::cerr << "Error:" << e.what() << std::endl ;
      return EXIT_FAILURE ;
   }
   return EXIT_SUCCESS ;
}
