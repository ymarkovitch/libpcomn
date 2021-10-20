###############################################################################
# A printer object must provide the following fileld:
#
# - printer_name:  A subprinter name used by gdb (required).
# - template_name: A string or a list of strings.
###############################################################################

import sys
import collections
import operator
import itertools
import gdb

import gdb.types, gdb.printing

from gdb import lookup_type, parse_and_eval

pcomn_printer_list = []
basic_printer_list = []

def consume(iterator):
    collections.deque(iterator, maxlen=0)

###############################################################################
# Pretty-printer registrars
###############################################################################
def register_printers(obj=None):
    """Register top-level printers 'pcomn' and 'basic' with objfile `obj`."""

    for category_name, category_list in (('pcomn', pcomn_printer_list),
                                         ('basic', basic_printer_list)):
        if not category_list:
            continue
        printers_category = PrettyPrinters(category_name)
        consume(map(printers_category.add, category_list))
        gdb.printing.register_pretty_printer(obj, printers_category, replace=True)

def add_printer(p):
    """Decorator that adds a printer to the top-level 'pcomn' printers."""

    pcomn_printer_list.append(p)
    return p

###############################################################################
# Printers meta-provider
###############################################################################
class PrettyPrinters(object):
    """Printer provider."""

    __slots__ = (
        'providers', # The list of printer providers
        'enabled',   # Enable/disable the whole set of printers provided by this object
        'name',      # The name of the set of printers (can be thought of as a category of printers)
        'template_printers',
        'basic_printers'
    )

    def __init__(self, name):
        self.name = name
        self.enabled = True
        self.providers = []

        self.template_printers = collections.defaultdict(list)
        self.basic_printers = []

    def __call__(self, value):
        wrapped_value = GDBValueWrapper(value)
        printer_constructors = self.template_printers.get(wrapped_value.template_name, self.basic_printers)

        def make_printer(constructor): constructor(wrapped_value)

        return next(itertools.dropwhile(operator.not_, map(make_printer, printer_constructors)), None)

    @property
    def subprinters(self):
        return self.providers

    def add(self, printer, typename=str()):
        if not hasattr(printer, 'supports') and not hasattr(printer, 'template_name') and not typename:
            message('cannot import printer {printer.printer_name!r}: neither supports() nor template_name is defined')
            return

        # Get the list of template names
        name_list = typename or getattr(printer, 'template_name', [])
        if isinstance(name_list, str):
            name_list = [name_list]
        if not isinstance(name_list, (list, tuple)):
            message(f"cannot import printer {printer.printer_name!r}: invalid type of template_name member {type(name_list)!r}")
            return

        # Create a new printer provider
        p = provider.PrinterProvider(printer, typename)
        # Put it to providers
        self.providers.append(p)
        # Append it to corresponding list
        if name_list:
            for template_name in name_list:
                self.template_printers[template_name].append(p)
        else:
            self.basic_printers.append(p)

    ###########################################################################
    # PrettyPrinters.PrinterProvider
    ###########################################################################
    class PrinterProvider(object):
        __slots__ = (
            'printer',  # Printer object
            'enabled',  # Indicates if the printer enabled
            'name',     # Printer name
            'supports'
        )

        def __init__(self, printer, typename=str()):
            assert typename != '' or hasattr(printer, 'printer_name')

            self.printer = printer
            # set printer_name
            self.name = typename or printer.printer_name
            # set enabled
            self.enabled = not hasattr(printer, 'enabled') or printer.enabled
            self.supports = getattr(printer, 'supports', lambda _: True)

        def __call__(self, value):
            return self.supports(value) and self.printer(value) or None

###############################################################################
# GDBValueWrapper derives from gdb.Value and provides the following extra members:
#  v.qualifiers
#  v.basic_type
#  v.type_name
#  v.template_name
###############################################################################
class GDBValueWrapper(gdb.Value):
    """Wrapper class for gdb.Value"""

    def __init__(self, value):
        gdb.Value.__init__(value)
        self.qualifiers = type_qualifiers(value.type)
        self.basic_type = gdb.types.get_basic_type(value.type)
        self.type_name = str(self.basic_type)
        self.template_name = template_name(self.basic_type)

###############################################################################
# Type qualifiers and template names handling
###############################################################################
#
# Get a string, which encodes const, volatile, and reference qualifiers of a gdb.Type
#
def type_qualifiers(t):
    """Get string encoding the qualifiers of a gdb.Type: const, volatile, reference.

    The result is a string where 'c' designates const, 'v' volatile, '&' reference.
    So e.g. 'const &foo' will return 'c&', 'const foo' will return 'c', etc.
    """
    assert isinstance(t, gdb.Type)
    t = t.strip_typedefs()

    qualifiers = t.code == gdb.TYPE_CODE_REF and '&' or ''

    if qualifiers:
        t = t.target()

    if t != t.unqualified():
        qualifiers += ('c'  if t == t.unqualified().const() else
                       'v'  if t == t.unqualified().volatile() else
                       'cv' if t == t.unqualified().const().volatile() else '')

    return qualifiers

#
# Get the template name of gdb.Type
#
def template_name(t):
    """Get the template name of gdb.Type. Only for struct/union/enum."""
    assert isinstance(t, gdb.Type)
    basic_type = gdb.types.get_basic_type(t)

    return (basic_type.code in [gdb.TYPE_CODE_STRUCT, gdb.TYPE_CODE_UNION, gdb.TYPE_CODE_ENUM] and
            str(basic_type).split('<')[0] or '')

def reinterpret_cast(value, target_type):
    return value.address.cast(target_type.pointer()).dereference()

def strip_qualifiers(typename):
    """Remove const/volatile qualifiers, references, and pointers of a type"""
    qps = []

    while True:
        typename = typename.rstrip()
        qual = next(itertools.dropwhile(lambda q: not typename.endswith(q), ('&', '*', 'const', 'volatile', '')))
        if not qual: break
        typename = typename[:-len(qual)]
        qps.append(qual)

    while True:
        typename = typename.lstrip()
        qual = next(itertools.dropwhile(lambda q: not typename.startswith(q), ('const', 'volatile', '')))
        if not qual: break
        typename = typename[len(qual):]
        qps.append(qual)

    return typename, qps[::-1]
