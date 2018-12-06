import numpy as np
from collections import namedtuple, Sequence
from .read_data import *
from .write_data import *
import logging


logger = logging.getLogger(__name__)


class Reductions(object):
    """
    This class bundles methods used for reduction operations over
    result files. These methods all take two values and return
    the reduction result. The two original values may be changed
    during the calculation.
    """

    @staticmethod
    def equal(a, b):
        """
        Reduction that insures that the value is the same in both objects.
        Otherwise an assertion will be triggered.
        """
        assert (np.asarray(a) == b).all()
        return a

    @staticmethod
    def add(a, b):
        """
        Adds the two values.
        If they are not scalar, they will be converted to numpy arrays.
        (So adding to lists of the same length will result
        in an array that contains the element-wise sum).
        """
        if not isinstance(a, numbers.Number):
            a = np.asarray(a)
        a += b
        return a

    @staticmethod
    def concat(a, b):
        """
        Concatenates a and b.
        """
        return np.concatenate((a, b))

    @staticmethod
    def fail(a, b):
        """
        Marks a value as non-reducible. Any attempt to reduce will result in a
        violated assertion.
        """
        raise AssertionError("Cannot reduce")


class DataSpec(namedtuple("_DataSpec", ("name", "type", "count", "is_attr", "reduction"))):
    """
    Specification for one entry of Data in a result file. This class defines the name
    of the entry, its data type and shape, and how it behaves in reduction operations.
    It also allows setting `is_attr` to False, which means that the data is supposed to
    be read from and written to a file, but not saved as an attribute in the target object.

    The properties `count` and `type` can define static values, or values that depend on
    already defined DataSpecs. The class is derived from a tuple, so its attributes cannot
    be changed after creation.

    To realise the file IO, the `read` and `write` functions can be used, which take in the
    data (for all DataSpecs of the target object) and perform io.
    """
    def __new__(cls, name, dtype, count=1, is_attr=True, reduction=Reductions.equal):
        if not callable(dtype) and not DataSpec.is_valid_dtype(dtype):
            raise TypeError("Invalid dtype {} specified".format(dtype))

        if isinstance(reduction, str):
            reduction = getattr(Reductions, reduction)
        if not callable(reduction):
            raise TypeError("Reduction operation {} is not callable".format(reduction))

        self = super(DataSpec, cls).__new__(cls, name, dtype, count, is_attr, reduction)
        return self

    @staticmethod
    def is_valid_dtype(type_):
        """
        helper function that checks whether `type_` is a valid data type for
        DataSpec objects. Valid data types are any numpy `dtype` objects or
        object that can be converted to numpy dtypes, `int`,
        `float` and `"grid"`.
        """
        # is it a numpy dtype?
        if isinstance(type_, np.dtype):
            return True

        # is it one of our predefined types?
        if type_ in (int, float, "grid"):
            return True

        # can we convert it to a numpy type?
        try:
            type_ = np.dtype(type_)
            return True
        except TypeError:
            pass

        # OK, seems to be an invalid type
        return False

    @staticmethod
    def resolve_count(count, data):
        """
        Turns the shape specification `count` into a shape tuple. Any textual entries in the
        specification are replaced by their values from data.
        
        :param Integral|str|Sequence count: Specification for an element count.
        :param dict data: Data dict, from which values can be taken to fill in placeholders in `count`.
        :return: A tuple of integers.
        :raises KeyError: If a placeholder in `count` was not found in `data`.
        :raises TypeError: If the type of `count` is not an integer, string, or sequence of those.
        """
        if isinstance(count, numbers.Integral):
            return count
        elif isinstance(count, str):
            if count not in data:
                raise KeyError("Number of items specified as {}, which is not present in data.".format(count))
            return data[count]
        elif isinstance(count, Sequence):
            return tuple(DataSpec.resolve_count(c, data) for c in count)
        else:
            raise TypeError("Invalid count {} specified".format(count))

    @staticmethod
    def resolve_dtype(type_, data):
        """
        Turns the data type specification `type_` into an actually usable data type.
        That means that if `type_` is a callable, the actual type will be determined
        by calling it with `data` as a parameter, otherwise the `type_` will be used
        directly.
        
        :param type|callable type_: Data type specification
        :param dict data: A dict of data information that may be used to determine the actual data type.
        :return: The data type that is specified by `type_`.
        """
        # if the specified dtype is not a valid dtype in itself, it means
        # we have a callable.
        if not DataSpec.is_valid_dtype(type_):
            type_ = type_(data)
            if DataSpec.is_valid_dtype(type_):
                return type_
            else:
                raise TypeError("Invalid data type {}".format(type_))

        return type_

    def __repr__(self):
        if not self.is_attr:
            return "DataSpec(%r, %r, %r, is_attr=False)" % (self.name, self.type, self.count)
        else:
            return "DataSpec(%r, %r, %r)" % (self.name, self.type, self.count)

    def read(self, file_, data):
        """
        Reads the entry specified by `self` from `file_` and puts the value into `data`.
        In case the reading causes the exception, additional information will be logged.
        
        :param BinaryIO file_: File from which to read.
        :param dict data: Dict in which to put the value. Data already present in the dict will /
            be used to determine dynamic read counts.
        :return: The value that was read.
        """
        try:
            return self._read(file_, data)
        except Exception as E:
            # pass through io exceptions, but log the corresponding DataSpec instance
            logger.error("An error %r occurred when reading data for spec '%s' (%r)", E, self.name, self)
            raise

    def _read(self, file_, data):
        """
        Implementation of read.
        """
        # get type and amount of data to read.
        type_ = self.resolve_dtype(self.type, data)
        shape = self.resolve_count(self.count, data)

        # and delegate to the corresponding reader.
        if type_ is "grid":
            result = read_grid(file_, shape)
        elif type_ is int:
            result = read_int(file_, shape)
        elif type_ is float:
            result = read_float(file_, shape)
        else:
            result = read_array(file_, type_, shape)

        data[self.name] = result
        return result

    def write(self, file_, data):
        """
        Writes the entry specified by `self` from `data` into `file_`.
        
        :param BinaryIO file_: File to which is written.
        :param dict data: Dict which contains the relevant values.
        :return: nothing.
        :raises KeyError: If `data` does not contain an entry for this spec.
        """
        try:
            return self._write(file_, data)
        except Exception as E:
            # pass through io exceptions, but log the corresponding DataSpec instance
            logger.error("An error %r occurred when writing data for spec '%s' (%r)", E, self.name, self)
            raise

    def _write(self, file_, data):
        """
        Implementation for the write function.
        """

        # first, resolve data type and number of elements to write
        type_ = self.resolve_dtype(self.type, data)
        shape = self.resolve_count(self.count, data)
        value = data[self.name]

        # and then delegate to the corresponding writer
        if type_ is "grid":
            if shape == 1:
                write_grid(file_, value)
            else:
                for grid in range(shape):
                    write_grid(file_, value[grid])
        elif type_ is int:
            write_int(file_, value, shape)
        elif type_ is float:
            write_float(file_, value, shape)
        else:
            value.tofile(file_)
