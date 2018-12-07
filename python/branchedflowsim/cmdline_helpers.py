from collections import namedtuple

ArgSpec = namedtuple("ArgSpec", ("name", "description", "positional", "required", "type", "amount"))


def _to_string(type_, value):
    if type_ == "Bool":
        return "1" if value else "0"
    else:
        return str(value)


class CmdLineBuilder(object):
    def __new__(cls, *args, **kwargs):
        if not hasattr(cls, "_ARGUMENTS_"):
            raise TypeError("No _ARGUMENTS_ found in %s (%s)" % (cls.__name__, cls))
        if not hasattr(cls, "_NAME_"):
            raise TypeError("No _NAME_ found in %s (%s)" % (cls.__name__, cls))
        return super(CmdLineBuilder, cls).__new__(cls, *args, **kwargs)

    @property
    def arg_spec(self):
        return getattr(self, "_ARGUMENTS_")

    @property
    def name(self):
        return getattr(self, "_NAME_")

    @property
    def args(self):
        opt_pos_missing = None
        args = []
        for attr in self.arg_spec:  # type: ArgSpec
            value = getattr(self, attr.name)

            if value is None:
                if attr.required:
                    raise ValueError("Argument '%s' not supplied for %s" % (attr.name, self.__class__.__name__))
                elif attr.positional:
                    opt_pos_missing = attr
                continue

            if attr.positional and opt_pos_missing:
                raise ValueError("Submitted a positional argument '%s' after empty optional '%s' not"
                                 " possible for %s" % (attr.name, opt_pos_missing.name,
                                                                self.__class__.__name__))

            # FLAGS
            if attr.type == "Flag":
                if value:
                    args.append(attr.name)
            else:
                if not attr.positional:
                    args.append(attr.name)

                if attr.amount == 1:
                    args.append(_to_string(attr.type, value))
                else:
                    args += map(lambda x: _to_string(attr.type, x), value)

        return args

    def make_command(self):
        return [self.name] + self.args

    @property
    def as_dict(self):
        """
        Gets the config of this CommandLineBuilder as a dictionary, so it can be
        saved to e.g. a json file.
        :return:
        """
        result = {}
        for attr in self.arg_spec:
            result[attr.name] = getattr(self, attr.name)
        return result

    # helper functions for derived classes
    @staticmethod
    def _verify_bool(value, arg_name):
        if value is None or isinstance(value, bool):
            return value

        raise ValueError("Expected boolean value for argument '%s', got %r" % (arg_name, value))

    @staticmethod
    def _verify_number(value, arg_name):
        if value is None:
            return value

        try:
            return float(value)
        except ValueError:
            raise ValueError("Expected real value for argument '%s', got %r" % (arg_name, value))
        except TypeError as e:
            raise TypeError(e.message + " for argument '%s'" % arg_name)

    @staticmethod
    def _verify_integer(value, arg_name):
        if value is None:
            return value

        try:
            iv = int(value)
            if type(value)(iv) == value:
                return iv
            else:
                raise ValueError()
        except ValueError:
            raise ValueError("Expected integer value for argument '%s', got %r" % (arg_name, value))
        except TypeError as e:
            raise TypeError(e.message + " for argument '%s'" % arg_name)

    @staticmethod
    def _verify_string(value, arg_name):
        if value is None:
            return value

        try:
            return str(value)
        except ValueError:
            raise ValueError("Expected string value for argument '%s', got %r" % (arg_name, value))

    @staticmethod
    def _verify_sequence(values, arg_name, verification):
        if values is None:
            return values

        # if values can be interpreted as a single value, use as one-element list
        try:
            return [verification(values, arg_name)]
        except:
            return list(map(lambda x: verification(x, arg_name), values))
