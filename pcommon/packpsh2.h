/*-*- mode: c; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel" -*-*/
/*******************************************************************************
 FILE         :   packpsh2.h
 COPYRIGHT    :   Yakov Markovitch, 1998-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   This file turns 2 byte packing of structures on.  (That is, it disables
                  automatic alignment of structure fields.)  An include file is needed
                  because various compilers do this in different ways.  For Microsoft
                  compatible compilers, this file uses the push option to the pack pragma
                  so that the poppack.h include file can restore the previous packing
                  reliably.
                  The file packpop.h is the complement to this file.

 CREATION DATE:   30 Jun 1998
*******************************************************************************/
#ifdef __BORLANDC__
#  pragma nopackwarning
#endif

#if ! (defined(lint) || defined(_lint))
#  ifndef RC_INVOKED
#    if ( _MSC_VER >= 800 ) || (defined(__BORLANDC__))
#      pragma warning(disable:4103)
#      if !(defined( MIDL_PASS )) || defined( __midl )
#        pragma pack(push)
#      endif
#      pragma pack(2)
#    else
#      pragma pack(2)
#    endif
#  endif // ndef RC_INVOKED
#endif // ! (defined(lint) || defined(_lint))
