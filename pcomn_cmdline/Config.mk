##########################################################################
## ^FILE: Config.mk - configuration flags for the CmdLine product
##
## ^DESCRIPTION:
##    This file is included in each makefile in the directory hierarchy.
##    It defines the Make macros used throughout the product build process.
##    These values represent the global defaults. At any level you run make,
##    you may override any of these by specifying a new value on the command
##    line.
##
## ^HISTORY:
##    04/28/92	Brad Appleton	<bradapp@enteract.com>	Created
###^^#####################################################################

   ## Universe to compile in (use "att" for SYSV Unix and "ucb" for BSD Unix).
.UNIVERSE=att

   ## Host operating system
OS=unix

#------------------------------------------------------------------------------
# Define some OS dependent stuff for file extensions
#
   ## C++ file extension
CEXT=.cpp

   ## object file extension
OBJEXT=.o

   ## library file extension
LIBEXT=.a

   ## executable file extension
EXECEXT=

#------------------------------------------------------------------------------
# Define some OS dependent stuff for directory names
#
   ## current working directory
CURDIR=./

   ## parent of current working directory
PARENTDIR=../

#------------------------------------------------------------------------------
# Define some common commands
#
   ## remove files
RM=rm -f

   ## remove directories
RMDIR=rm -rf

   ## create directories
MKDIR=mkdir -p

   ## change directories
CHDIR=cd

   ## echo current directory
PWD=pwd

   ## print to standard output
ECHO=echo

   ## copy files
CP=cp

   ## move/rename files
MV=mv

   ## print file on standard output
CAT=cat

   ## maintain archives
AR=ar r

   ## maintain library archives
RANLIB=ranlib

   ## remove symbol table info
STRIP=strip

   ## filter to remove non-ascii characters
COL=col -xb

   ## eliminate includes from a manpage
SOELIM=soelim

   ## compress a manpage
MANTOCATMAN=mantocatman

   ## print docs on printer
TROFF=roff -man

   ## print docs to screen
NROFF=nroff -man

   ## spell-checker
SPELL=spell

#------------------------------------------------------------------------------
# Define some target directories (where things are installed)
#
#    NOTE: if you change BINDIR, INCDIR, or LIBDIR then dont forget to change
#          their corresponding strings at the end of doc/macros.man!
#
   ## common local directory
LOCAL=/usr/local/

   ## where to install executables
BINDIR=$(LOCAL)bin/

   ## where to install object libraries
LIBDIR=$(LOCAL)lib/

   ## where to install object include files
INCDIR=$(LOCAL)include/

   ## where to install perl libraries
PERLLIB=$(LIBDIR)perl/

   ## where to install tcl libraries
TCLLIB=$(LIBDIR)tcl/

   ## subdirectory where local man-pages are installed
LOCALMAN=local_man/

   ## where to install man-pages
MANDIR=/usr/man/$(LOCALMAN)

   ## where to install catman-pages (preformatted manual pages)
CATMANDIR=/usr/catman/$(LOCALMAN)

   ## subdirectory of MANDIR and CATMANDIR for section 1 of man-pages
MAN1DIR=man1/

   ## subdirectory of MANDIR and CATMANDIR for section 3 of man-pages
MAN3DIR=man3/

#------------------------------------------------------------------------------
# Define some product specific stuff
#
PROGLIBDIR=$(PARENTDIR)lib/
PROGNAME=cmdparse
LIBNAME=cmdline

#------------------------------------------------------------------------------
# Define C++ compilation stuff
#
   ## name of C++ compiler
CC=c++

   ## option to specify other include directories to search
INC=-I

   ## option to specify constants to #define
DEF=-D

   ## option to produce optimized code
OPT=-O

   ## option to produce debugging information
DBG=-g -O

   ## option to indicate the name of the executable file
EXE=-o

   ## option to suppress the loading phase
NOLD=-c

#------------------------------------------------------------------------------
# Define rules for C++ files
#
.SUFFIXES : $(CEXT) $(OBJEXT)

$(CEXT)$(OBJEXT):
	$(CC) $(CFLAGS) $(NOLD) $<

#------------------------------------------------------------------------------
# Define some configurable compilation flags
#
# FLAG=$(OPT)
FLAG=$(DBG)
TESTDEFS=
USRDEFS=$(DEF)DEBUG_CMDLINE
OPTIONS=

#------------------------------------------------------------------------------
# Define the macro to pass to recursive makes
#
RECUR= "FLAG=$(FLAG)" \
   "TESTDEFS=$(TESTDEFS)" \
   "USRDEFS=$(USRDEFS)" \
   "OPTIONS=$(OPTIONS)"

#------------------------------------------------------------------------------
# Define the command for recursive makes
#
BUILD=$(MAKE) -$(MAKEFLAGS) $(RECUR)
