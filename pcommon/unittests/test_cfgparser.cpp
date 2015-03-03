/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_cfgparser.cpp
 COPYRIGHT    :   Yakov Markovitch, 1998-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Profile files (.INI) handling tests

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   29 May 1998
*******************************************************************************/
#include <pcomn_cfgparser.h>

#include <iostream>
#include <typeinfo>
#include <stdexcept>

static void usage()
{
   std::cout <<  "Profile file functions test." << std::endl
             << "Usage: prttst <profile_file_name> [section_name [value_name [value]]]"
             << std::endl ;
}

static int setValue (const char *filename, const char *section, const char *key, const char *value)
{
   if (!*key)
      key = NULL ;
   if (!*value)
      value = NULL ;

   return cfgfile_write_value(filename, section, key, value) ;
}

static int printValue (const char *filename, const char *section, const char *key, bool header = true)
{
   char buf[PCOMN_CFGPARSER_MAXLINEBUF] ;
   size_t result = cfgfile_get_value(filename, section, key, buf, "") ;

   if (header)
      std::cout << "File: " << filename << " Section: " << section
                << " Key: " << key << " Result: " << result
                << std::endl << '[' << section << ']' << std::endl ;

   std::cout << key << " = " << buf << std::endl ;
   if (header)
      std::cout << key << " = " << cfgfile_get_intval(filename, section, key, -1) << std::endl ;

   return 0 ;
}

static int printSection (const char *filename, const char *section, bool header = true)
{
   char buf[PCOMN_CFGPARSER_MAXLINEBUF] ;
   size_t result = cfgfile_get_value(filename, section, NULL, buf, "") ;

   if (header)
      std::cout << "File: " << filename << " Section: " << section << " Result: " << result
                << std::endl << std::endl ;

   std::cout << '[' << section << ']' << std::endl ;

   const char *s ;
   for (s = buf ; *s ; s += strlen(s)+1)
      printValue (filename, section, s, false) ;

   return 0 ;
}

static int printFile (const char *filename)
{
   char buf[PCOMN_CFGPARSER_MAXLINEBUF] ;

   std::cout << "File: " << filename << std::endl
             << " Result: "
             << cfgfile_get_value(filename, NULL, NULL, buf, "")
             << std::endl << std::endl ;

   std::cout << "Sections: " << std::endl ;

   const char *s ;
   for (s = buf ; *s ; s += strlen(s)+1)
      std::cout << '[' << s << ']' << std::endl ;

   std::cout << std::endl << "Values: " << std::endl ;
   for (s = buf ; *s ; s += strlen(s)+1)
      printSection (filename, s, false) ;

   return 0 ;
}


int main (int argc, char *argv[])
{
   try
   {
      switch(argc)
      {
         case 5 :
            return setValue(argv[1], argv[2], argv[3], argv[4]) ;

         case 4 :
            return printValue(argv[1], argv[2], argv[3]) ;

         case 3 :
            return printSection(argv[1], argv[2]) ;

         case 2 :
            return printFile(argv[1]) ;
      }
   }
   catch (const std::exception &x)
   {
      std::cout << "Exception of type [" << typeid(x).name() << "] thrown." << std::endl
                << x.what() << std::endl ;

      return 255 ;
   }

   usage() ;
   return 1 ;
}
