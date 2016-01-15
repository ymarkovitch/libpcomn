/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   test_crc32.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   The test of CRC32.
                  The program calculates the CRC32 for every file in the command line, or
                  for stdin, if the command line is empty.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   7 Dec 2009
*******************************************************************************/
#include <pcomn_hash.h>
#include <pcomn_fileutils.h>
#include <pcomn_sys.h>
#include <pcomn_unistd.h>

#include <fcntl.h>
#include <stdio.h>

using namespace pcomn ;

static void calculate(int fd)
{
   void *buf ;
   const ssize_t sz = pcomn::readfile(fd, NULL, 64*1024, &buf) ;
   if (sz < 0)
      puts(strerror(errno)) ;
   else
      printf("%X %lu\n", (unsigned)calc_crc32(0, (uint8_t *)buf, sz), (unsigned long)sz) ;
}

int main(int argc, char *argv[])
{
   if (argc < 2)
      calculate(fileno(stdin)) ;
   else
      for (int n = 1 ; n < argc ; ++n)
      {
         const int fd = open(argv[n], O_RDONLY) ;
         if (fd < 0)
            printf("Error opening '%s': %s\n", argv[n], strerror(errno)) ;
         else
         {
            calculate(fd) ;
            close(fd) ;
         }
      }
   return 0 ;
}
