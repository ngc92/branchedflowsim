from codegen import *
from collections import namedtuple

ArgSpec = namedtuple("ArgSpec", ("name", "description", "positional", "required", "type", "amount"))


def add_arguments(arguments, class_, init_function):
    """
    Add a list of arguments to a CommandLineBuilder class. This adds
    these (CLI-)arguments as arguments to the `__init__` method and
    corresponding getters and setters that validate the argument type.
    :param list[ArgSpec] arguments: Arguments of the class
    :param class_: The class to which the arguments are added.
    :param init_function: Init method of the class.
    :return:
    """
    for arg in arguments:
        argtype = None

        if arg.type == "Bool" or arg.type == "Flag":
            verify = "self._verify_bool"
            argtype = "bool"
        elif arg.type == "Number":
            verify = "self._verify_number"
            argtype = "float"
        elif arg.type == "Integer":
            verify = "self._verify_integer"
            argtype = "int"
        elif arg.type == "String":
            verify = "self._verify_string"
            argtype = "str"
        else:
            raise ValueError("Unknown argument type {}".format(arg.type))

        # use verify_sequence instead of verify for sequence arguments, and set the type hint correctly
        if arg.amount == -1:
            verify = "self._verify_sequence({1}, '{2}', " + verify + ")"
            argtype = "list[%s]" % argtype
        else:
            verify = verify + "({1}, '{2}')"
        assignment = "self._{0} = " + verify

        if arg.required:
            init_function.add_argument(Arg(arg.name, type_hint=argtype))
        else:
            init_function.add_argument(Arg(arg.name, "None", type_hint=argtype))

        init_function.add_code(assignment.format(arg.name, arg.name, arg.name))

        # a property to get the value
        getter = Function(arg.name)
        getter.add_decorator("@property")
        getter.add_argument("self")
        getter.docstring = arg.description
        getter.add_code("return self._%s" % arg.name)
        class_.add_method(getter)

        # and the corresponding setter
        setter = Function(arg.name)
        setter.add_decorator("@%s.setter" % arg.name)
        setter.add_argument("self")
        setter.add_argument(Arg("value", type_hint=argtype))
        setter.docstring = arg.description
        setter.add_code(assignment.format(arg.name, "value", arg.name))

        class_.add_method(setter)
