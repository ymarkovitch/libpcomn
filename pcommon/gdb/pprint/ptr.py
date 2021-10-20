########################################
# Pointer-likes
########################################
from utils import *

class PComnPrinter(object):
    """The base class for pcommon pretty printers"""

    __slots__ = 'value' ;

    def __init__(self, value):
        assert isinstance(value, GDBValueWrapper)
        self.value = value

    @property
    def typename(self):
        return self.value.type_name

#@add_printer
class PComnSharedPtr(PComnPrinter):
    """Pretty printer for pcomn::shared_intrusive_ptr"""

    printer_name = 'pcomn::shared_intrusive_ptr'
    template_name = 'pcomn::shared_intrusive_ptr'

    def __init__(self, value):
        super().__init__(value)

    def to_string(self):
        elem = self.element
        return elem != 0 and f"count {0}" or 'nullptr' ;

    def children(self):
        elem = self.element
        if elem != 0:
            yield 'value', elem.dereference()

    @property
    def element(self):
        return self.value['_object']
