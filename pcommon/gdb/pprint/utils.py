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
import gdb.types
import gdb.printing

from gdb import lookup_type, parse_and_eval

__all__ = [
    'GDBValue',

    'consume',
    'strip_qualifiers',
    'template_name'
]

def consume(iterator):
    """Consume the iterator entirely, return None."""
    collections.deque(iterator, maxlen=0)

###############################################################################
# GDBValue derives from gdb.Value and provides the following extra members:
#  v.qualifiers
#  v.basic_type
#  v.type_name
#  v.template_name
###############################################################################
class GDBValue(gdb.Value):
    """Wrapper class for gdb.Value"""

    __slots__ = (
        'qualifiers',
        'basic_type',
        'type_name',
        'template_name'
    )

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
# Get the template name of gdb.Type
#
def template_name(t):
    """Get the template name of gdb.Type. Only for struct/union/enum."""
    assert isinstance(t, gdb.Type)
    basic_type = gdb.types.get_basic_type(t)

    return (basic_type.code in [gdb.TYPE_CODE_STRUCT, gdb.TYPE_CODE_UNION, gdb.TYPE_CODE_ENUM] and
            str(basic_type).split('<')[0] or '')

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
