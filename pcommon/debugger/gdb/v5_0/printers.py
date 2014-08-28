#
# FILE         :  printers.py
# COPYRIGHT    :  Yakov Markovitch, 2009-2014. All rights reserved.
#                 See LICENSE for information on usage/redistribution.
# DESCRIPTION  :  GDB pretty-printers for PCOMMON
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  3 Dec 2009
#

__all__ = ["register_printers"]

import gdb, re

class ValuePrinter(object):
    def __init__(self, typename, val):
        self.typename = typename
        self.val = val

class PTSafePtrPrinter(ValuePrinter):
    """Print a safe pointer"""

    def __init__ (self, typename, val):
        super(PTSafePtrPrinter, self).__init__(typename, val)

    def to_string (self):
        v = self.val['_ptr']
        t = str(v.type.target())
        return "(PTSafePtr<%s%s>) %s" % (t, t[-1]=='>' and ' ' or '', v)

class PTSmartPtrPrinter(ValuePrinter):
    """Print a smart pointer (reference-counted shared pointer)"""

    def __init__ (self, typename, val):
        super(PTSmartPtrPrinter, self).__init__(typename, val)

    def to_string(self):
        p = self.ptr()
        return \
            ("(%s count %d) %s" % (self.typename, self.refcount(), p)) if p else ('(%s) NULL' % (self.typename,))

    def ptr(self): pass
    def refcount(self): pass

class PTDirectSmartPtrPrinter(PTSmartPtrPrinter):
    """Print a direct smart pointer"""

    def __init__ (self, typename, val):
        super(PTDirectSmartPtrPrinter, self).__init__(typename, val)

    # Implement the method which is "abstract" in the base class
    def ptr(self):
        return self.value['_ref']['_object']

    def refcount(self):
        p = self.value['_ref']['_object']
        return p['_counter'] if p else 0

class PTIndirectSmartPtrPrinter(PTSmartPtrPrinter):
    """Print an indirect smart pointer"""

    def __init__ (self, typename, val):
        super(PTIndirectSmartPtrPrinter, self).__init__(typename, val)

    # Implement the method which is "abstract" in the base class
    def ptr(self):
        r = self.value['_ref']['_reference']
        return r and r['_object']

    def refcount(self):
        return self.value['_ref']['_reference']['_counter']

###############################################################################
# Pretty-printers dictionary
###############################################################################
extract_type_prefix = re.compile(r"^(pcomn::(?:[a-zA-Z_]\w*::)*[a-zA-Z_]\w*)(?:<.*>)?$").match

pretty_printers = {
    "pcomn::PTSafePtr" : ((re.compile(r"^pcomn::PTSafePtr<.+, pcomn::ScalarPtr>$").match, PTSafePtrPrinter),)
    "pcomn::PTDirectSmartPtr" : ((re.compile(r"^pcomn::PTDirectSmartPtr<.+>$").match, PTDirectSmartPtr),)
    "pcomn::PTIndirectSmartPtr" : ((re.compile(r"^pcomn::PTIndirectSmartPtr<.+>$").match, PTIndirectSmartPtr),)
    }

def pcomn_type_prefix(type_name):
    pfx = type_name is not None and extract_type_prefix(type_name)
    return pfx and pfx.group(1)

def lookup_function(val):
    "Look-up and return a pretty-printer that can print val."

    # Get the target type.
    target_type = val.type
    # If it points to a reference, get the reference.
    if target_type.code == gdb.TYPE_CODE_REF:
        target_type = target_type.target()

    # Get the name of unqualified type, stripped of typedefs.
    typename = target_type.unqualified().strip_typedefs().tag

    for pp in pretty_printers.get(pcomn_type_prefix(typename), ()):
        if pp[0](typename):
            return pp[1](typename, val)
    # Cannot find a pretty printer.  Return None.

def register_printers(obj):
    "Register PCOMMON pretty-printers with objfile obj."

    if obj == None:
        obj = gdb
    lookup_function in obj.pretty_printers or obj.pretty_printers.append(lookup_function)
