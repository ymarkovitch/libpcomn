###############################################################################
# Register pcommon pretty printers.
#
# A printer object must provide the following fileld:
#   - printer_name:  A subprinter name used by gdb (required).
#   - template_name: A string or a list of strings.
###############################################################################
import gdb
import gdb.printing

import collections
import itertools
import operator

from .utils import *

__all__ = [
    'PComnPrinter',

    'add_printer',
    'register_printers'
    ]

pcomn_printer_list = []
basic_printer_list = []

###############################################################################
# Pretty-printer registrars
###############################################################################
def add_printer(p):
    """Decorator that adds a printer to the top-level 'pcomn' printers."""

    pcomn_printer_list.append(p)
    return p

def register_printers(obj=None):
    """Register top-level printers 'pcomn' and 'basic' with objfile `obj`."""

    for category_name, category_list in (('pcomn', pcomn_printer_list),
                                         ('basic', basic_printer_list)):
        if not category_list:
            continue
        printers_category = PrettyPrinters(category_name)
        consume(map(printers_category.add, category_list))
        gdb.printing.register_pretty_printer(obj, printers_category, replace=True)


###############################################################################
# The base class for pcommon pretty printers
###############################################################################
class PComnPrinter(object):
    """The base class for pcommon pretty printers."""

    __slots__ = 'value' ;

    def __init__(self, value):
        assert isinstance(value, GDBValue)
        self.value = value

    @property
    def typename(self):
        return self.value.type_name

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
        wrapped_value = GDBValue(value)
        printer_constructors = self.template_printers.get(wrapped_value.template_name, self.basic_printers)

        def make_printer(constructor):
            return constructor(wrapped_value)

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
        p = PrettyPrinters.PrinterProvider(printer, typename)
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
            'supports'  # A callable, if present, must answer if this provider supports the given value.
        )

        def __init__(self, printer, typename=str()):
            assert typename != '' or hasattr(printer, 'printer_name')

            self.printer = printer
            # set printer_name
            self.name = typename or printer.printer_name
            # set enabled
            self.enabled = not hasattr(printer, 'enabled') or printer.enabled
            # set enabled
            self.supports = getattr(printer, 'supports', lambda _: True)

        def __call__(self, value):
            return self.supports(value) and self.printer(value) or None
