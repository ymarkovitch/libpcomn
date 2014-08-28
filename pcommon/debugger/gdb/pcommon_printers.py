#
# FILE         :  pcommon_printers.py
# COPYRIGHT    :  Yakov Markovitch, 2009-2014. All rights reserved.
#                 See LICENSE for information on usage/redistribution.
# DESCRIPTION  :  The loader of GDB pretty-printers package for PCOMMON
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  3 Dec 2009
#

"""The loader module of GDB pretty-printers package for PCOMMON objects"""

__all__ = []
import sys, os.path

def init_printers(anchor=(lambda:1)):
    selfpath = os.path.dirname(os.path.abspath(anchor.func_code.co_filename))
    (selfpath in sys.path) or sys.path.insert(0, selfpath)

init_printers()
import pcommon
