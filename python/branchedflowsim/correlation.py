import numpy as np


class Correlation(object):
    """
    Base class for all correlation functions. To customize the specification
    derived classes can override the `_args` function.
    """
    __CONFIG__ = ["kind", "scale"]

    def __init__(self, kind, scale):
        """
        :param str kind: The kind of correlation to represent.
        :param float scale: The global length scale of the correlation.
        """
        self._kind = kind
        self._scale = scale

    @property
    def scale(self):
        """
        Global length scale os the correlation function.
        """
        return self._scale

    @property
    def kind(self):
        """
        The kind of correlation function to create as a string. This is what is passed as the `-c` option to the
        `potgen` program. Usually set to the correct type by a derived class.
        """
        return self._kind

    @property
    def args(self):
        """
        Gets a string list that is used to specify this correlation function
        when communicating with the c++ programme.
        :rtype list[str] 
        """
        return ["-c", self.kind] + self._args() + ['-l', str(self.scale)]

    def _args(self):
        return []

    @property
    def as_dict(self):
        """
        Returns the parameters of the Correlation function as a dictionary, so that they may be saved into a json file.
        """
        args = {}
        for attribute in self.__CONFIG__:
            if hasattr(self, attribute):
                args[attribute] = getattr(self, attribute)
            elif hasattr(self, "_"+attribute):
                args[attribute] = getattr(self, "_" + attribute)

        return {"type": self.__class__.__name__, "params": args}

    @staticmethod
    def from_dict(data):
        """
        Reconstructs the correlation function from the dict representation.
        :param data: A dictionary with the same structure as returned by `as_dict`. 
        :return: A new Correlation object.
        """
        spec_class = data["type"]
        for sub in Correlation.__subclasses__():
            if sub.__name__ == spec_class:
                return sub(**data["params"])
        raise TypeError("Could not find '%s' subclass of 'Correlation'" % spec_class)

    def __eq__(self, other):
        """
        Checks whether two Correlation functions are equal. They are considered equal if their `as_dict` representations 
        are equal.
        """
        if not isinstance(other, Correlation):
            return False
        return self.as_dict == other.as_dict

    def __repr__(self):
        values = map(lambda x: "%s = %s" % x, self.as_dict.items())
        return self.__class__.__name__ + "(%s)" % ",".join(values)


class Parametrized(Correlation):
    __CONFIG__ = ["kind", "scale", "arg"]

    """
    A correlation function that has a configurable parameter `arg`.
    """
    def __init__(self, kind, scale, arg):
        """
        :param str kind: The kind of correlation to represent.
        :param float scale: The global length scale of the correlation.
        :param arg: Additional argument that will be supplied to the correlation function.
        """
        super(Parametrized, self).__init__(kind, scale)
        self._arg = arg

    def _args(self):
        """ creates a string list that is used to specify this correlation function
            when communicating with the c++ programme
        """
        cf = []
        if self._arg is not None:
            cf += [str(self._arg)]
        return cf


class IsotropicGaussian(Correlation):
    """
    Isotropic gaussian correlation.
    """
    __CONFIG__ = ["scale"]

    def __init__(self, scale):
        """
        :param float scale: The length scale of the gaussian correlation.
        """
        super(IsotropicGaussian, self).__init__("gauss", scale)


class AnisotropicGaussian(Correlation):
    """
    Anisotropic gaussian correlation. Assumes that the anisotropy is axis-aligned.
    For a length scale `s` and anisotropy factors `a`, `b` the transformation matrix
    `M` is given by `M = diag(a,b) / s` and the correlation function is `c(x) = exp(-x'(M'M)x)`.
    """

    __CONFIG__ = ["scale", "factors"]

    def __init__(self, scale, factors):
        """
        :param float scale: The global length scale of the correlation.
        :param Sequence[float] factors: The scale factors for the different axes.
        """
        super(AnisotropicGaussian, self).__init__("gauss", scale)
        self._factors = tuple(factors)

    @property
    def scale_factors(self):
        return self._factors

    def _args(self):
        """ creates a string list that is used to specify this correlation function
            when communicating with the c++ programme
        """
        cf = list(map(str, self._factors))
        return cf


class CustomScript(Correlation):
    """
    Correlation function based on a give lua script.
    """

    __CONFIG__ = ["scale", "script_file", "script_args"]

    def __init__(self, scale, script_file, script_args=None):
        """
        :param float scale: The global length scale of the correlation.
        :param str script_file: Path to the file that contains the lua script.
        :param dict script_args: Dictionary with (numerical) values that should be set
                                 inside the lua script.
        """
        super(CustomScript, self).__init__("lua", scale)
        if script_args is None:
            script_args = {}
        self._script_file = script_file
        self._script_args = script_args

    @property
    def script_file(self):
        return self._script_file

    @property
    def parameters(self):
        return self._script_args

    def _args(self):
        """ creates a string list that is used to specify this correlation function
            when communicating with the c++ programme
        """
        params = [self._script_file]
        for name in self._script_args:
            params += [name, self._script_args[name]]

        cf = list(map(str, params))
        return cf


class Transformed(Correlation):
    """
    Correlation function that is based on a coordinate transformation of another correlation function.
    """
    __CONFIG__ = ["base", "transform"]

    def __init__(self, base, transform):
        """
        :param Correlation base: Original correlation.
        :param np.array transform: Transformation matrix.
        """
        super(Transformed, self).__init__(base.kind, base.scale)
        self._base = base
        self._transform = np.asarray(transform)

    @property
    def base_correlation(self):
        return self._base

    @property
    def transform(self):
        return self._transform

    def _args(self):
        """ creates a string list that is used to specify this correlation function
            when communicating with the c++ programme
        """
        spec = " ".join(map(str, self._transform.flat))
        return self._base._args() + ["--trafo", '"%s"' % spec]

