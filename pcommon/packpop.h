/*-*- mode: c; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel" -*-*/
/*******************************************************************************
 FILE         :   packpop.h
 COPYRIGHT    :   Yakov Markovitch, 1998-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   This file turns packing of structures off.  (That is, it enables
                  automatic alignment of structure fields.)  An include file is needed
                  because various compilers do this in different ways.
                  packpop.h is the complement to packpsh?.h.  An inclusion of packpop.h
                  MUST ALWAYS be preceded by an inclusion of one of packpsh?.h, in one-to-one
                  correspondence.
                  For Microsoft compatible compilers, this file uses the pop option
                  to the pack pragma so that it can restore the previous saved by the
                  packpsh?.h include file.

 CREATION DATE:   30 Jun 1998
*******************************************************************************/
#ifdef __BORLANDC__
#  pragma nopackwarning
#endif
#ifdef _MSC_VER >= 800
#  pragma warning(disable:4103)
#endif

#pragma pack(pop)
