import textwrap
from copy import copy


# small codegen helpers
class Class(object):
    def __init__(self, name, base=None, docstring=None):
        self._name = name
        if base is None:
            base = "object"
        self._base = base
        self._docstring = None
        self._methods = []
        self._code = []
        if docstring is not None:
            self.docstring = docstring

    @property
    def docstring(self):
        return self._docstring

    @docstring.setter
    def docstring(self, value):
        if isinstance(value, Docstring):
            self._docstring = value
        else:
            self._docstring = Docstring(value)

    def add_method(self, method):
        assert isinstance(method, Function)
        self._methods.append(method)

    def add_code(self, code):
        self._code.append(code)

    def __str__(self):
        fmt = "class %s(%s):\n"
        string = fmt % (self._name, self._base)
        if self._docstring:
            string += indent(self._docstring) + "\n"

        for code in self._code:
            string += indent(code) + "\n"

        for method in self._methods:
            string += indent(method) + "\n"
        return string

    def __repr__(self):
        return "<class %s(%s)>" % (self._name, self._base)


def indent(target, level=1):
    indentation = " " * (4 * level)
    ts = indentation + str(target)
    return ts.replace("\n", "\n" + indentation)


class Docstring(object):
    def __init__(self, value):
        self._doc = value  # type: str

    def add_paragraph(self, par):
        self._doc += "\n\n" + par

    def __str__(self):
        docstr = '"""\n%s\n"""\n'
        wrapped = self._doc.split("\n")
        wrapped = "\n".join(map(lambda x: textwrap.fill(x, 80), wrapped))
        # TODO this wrapping disregards any indentation we will perform on the docstring.
        return docstr % wrapped

    def __repr__(self):
        return "Docstring('%s')" % self._doc.replace('"', "'")


class Arg(object):
    def __init__(self, name, default=None, type_hint=None):
        self._name = name
        self._default = default
        self._type_hint = type_hint

    def __str__(self):
        if self._default is not None:
            return "%s=%s" % (self._name, self._default)
        else:
            return self._name


class Function(object):
    def __init__(self, name):
        self._name = name
        self._args = []
        self._decorators = []
        self._docstring = None
        self._code = []

    @property
    def docstring(self):
        return self._docstring

    @docstring.setter
    def docstring(self, value):
        if isinstance(value, Docstring):
            self._docstring = value
        else:
            self._docstring = Docstring(value)

    def add_argument(self, arg):
        if isinstance(arg, str):
            arg = Arg(arg)
        self._args.append(arg)

    def add_decorator(self, decorator):
        self._decorators.append(decorator)

    def add_code(self, code):
        self._code.append(code)

    def __str__(self):
        pos_args = [arg for arg in self._args if arg._default is None]
        kwargs = [arg for arg in self._args if arg._default is not None]
        # positional arguments come before kwargs!
        args = ", ".join(map(str, pos_args + kwargs))
        fmt = "def %s(%s):\n"
        if self._decorators is not None:
            decs = "\n".join(self._decorators) + "\n"
        else:
            decs = ""
        string = decs + fmt % (self._name, args)

        docstr = copy(self._docstring)  # type: Docstring

        type_hints = [arg for arg in self._args if arg._type_hint is not None]
        if len(type_hints) > 0:
            fmt = ":type %s: %s"
            ths = [fmt % (arg._name, arg._type_hint) for arg in type_hints]
            type_hints = "\n".join(ths)

            if not docstr:
                docstr = Docstring(type_hints)
            elif docstr:
                docstr.add_paragraph(type_hints)

        if docstr:
            string += indent(docstr) + "\n"

        for code in self._code:
            string += indent(code) + "\n"

        if len(self._code) == 0:
            string += indent("pass")

        return string
