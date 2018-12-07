from collections import namedtuple
import logging

import numpy as np
from branchedflowsim.io import read_int, read_float, read_grid, write_int, write_float, write_grid

_logger = logging.getLogger(__name__)
PotCfg = namedtuple("PotCfg", ("dimension", "support", "extents", "grids", "strength", "meta"))


def _read_pot_config(source_file):
    """
    reads the metadata portion of the Potential in `source_file`.
    :param file source_file: An open file object (binary) where the potential resides.
    :return:
    """
    source_file.seek(0)
    assert(source_file.read(5) == 'bpot5')  # check grid
    hs = int(source_file.readline())
    info = source_file.read(hs - 1)   # human readable info string.
    # subtract 1 because the c++ impl inludes the \n in its count

    dimension = read_int(source_file)
    support = read_float(source_file, count=dimension)
    extents = read_int(source_file, dimension)
    seed = read_int(source_file)
    potgen = read_int(source_file)
    gridcount = read_int(source_file)
    corrlength = read_float(source_file, count=1)
    strength = read_float(source_file, count=1)

    meta = {"seed": seed, "corrlength": corrlength, "version": potgen, "info": info}

    return PotCfg(dimension, support, tuple(extents), gridcount, strength, meta)


def _read_fields(source_file, target_potential, count):
    """
    Reads `count` many fields from `source_file` and saves them in `target_potential`.
    :param file source_file:
    :param Potential target_potential:
    :param int count:
    """
    for i in range(0, count):
        # name
        len = read_int(source_file)
        name = "".join(map(chr, np.fromfile(source_file, dtype=np.uint8, count=len)))

        field = target_potential.get_field(name)
        index = tuple(read_int(source_file, target_potential.dimension))
        # calculate index
        field.set_partial_derivative(read_grid(source_file), *index)


class Field(object):
    """
    A `Field` represents a single physical scalar field.
    It may contain precomputed spatial partial derivatives.
    """
    def __init__(self, extents):
        self._data = {}
        self._extents = tuple(extents)

    @property
    def dimension(self):
        return len(self.extents)

    @property
    def extents(self):
        """
        Get the extents of this Field, i.e. the upper bounds for valid indices into
        the data arrays.
        """
        return self._extents

    def derivatives(self):
        """
        Gets a view to the saved derivatives. The resulting view
        allows to iterate over tuples describing the derivatives
        present in this Field.
        """
        return self._data.viewkeys()

    @property
    def field(self):
        """
        Gets the field itself, i.e. the zeroth derivative.
        """
        return self.get_partial_derivative(*((0,) * self.dimension))

    def get_partial_derivative(self, *derivatives):
        """
        Gets a partial derivative of the field. The arguments passed as `derivatives` determine how many times derivation
        should have taken place w.r.t. that given index. For example for a field `V` we have
        `V.get_partial_derivative(1, 0) == dV/dx` and `V.get_partial_derivative(0, 2) == d^2V/dy^2`.
        """
        derivatives = tuple(derivatives)

        if len(derivatives) != self.dimension:
            raise ValueError("Wrong amount of derivative arguments. Expected %s but got %s" %
                             (self.dimension, len(derivatives)))

        return self._data[derivatives]

    def set_partial_derivative(self, value, *derivatives):
        """
        Sets a partial derivative of the field. The arguments passed as `derivatives` determine how many times derivation
        should have taken place w.r.t. that given index. For example for a field `V` we have
        `V.get_partial_derivative(1, 0) == dV/dx` and `V.get_partial_derivative(0, 2) == d^2V/dy^2`.
        :raises: ValueError, if the shape of `value` does not match the `extents` of this Field.
        """
        derivatives = tuple(derivatives)

        value = np.asarray(value)
        if value.shape != self.extents:
            raise ValueError("Shape %s of partial derivative does not match shape of field %s" %
                             (value.shape, self.extents))

        self._data[derivatives] = value


class Potential(object):
    """
    A potential represents a single realisation of a `medium`. It corresponds almost directly
    to the c++ `Potential` class.
    """
    def __init__(self, config):
        """
        :param config: Potential config object. Should contain the attributes `dimension`, `support`, \
                       `extents`, `meta`, `strength`.
        """
        self._dimension = config.dimension
        self._support = config.support
        self._extents = tuple(config.extents)
        self._meta = config.meta
        self._strength = config.strength
        self._fields = {}
        self._file_name = None

    @staticmethod
    def from_file(source_file):
        """
        Creates a potential with the contents of `source_file`. Note that this
        function performs lazy loading. Therefore it is unsave to delete the
        `source_file` before the data has been accessed through the returned
        Potential object.
        :return: A Potential object that loads field data on demand.
        """
        return LazyPotential(source_file)

    def to_file(self, target_file):
        """
        Writes the potential to `target_file`.
        :param target_file: An open file object (binray mode) into which the potential will be written.
        :return: nothing.
        """
        _logger.info("Saving potential data to file %s.", target_file.name)

        num_grids = 0
        for key in self._fields:
            num_grids += len(self.get_field(key).derivatives())

        target_file.write("bpot5 ")
        target_file.write(str(len(self._meta.get("info", ""))+1)+"\n")
        target_file.write(self._meta.get("info", ""))
        write_int(target_file, self.dimension)
        write_float(target_file, self.support, self.dimension)
        write_int(target_file, self.extents, self.dimension)
        write_int(target_file, self._meta.get("seed", 0))
        write_int(target_file, self._meta.get("version", 0))
        write_int(target_file, num_grids)
        write_float(target_file, self._meta.get("corrlength", 0))
        write_float(target_file, self.strength)

        for key in self._fields:
            field = self.get_field(key)

            for derivative in field.derivatives():
                write_int(target_file, len(key))
                target_file.write(key)
                write_int(target_file, derivative, self.dimension)
                write_grid(target_file, np.asanyarray(field.get_partial_derivative(*derivative), np.float64))

        self._file_name = target_file.name

    @property
    def file_name(self):
        return self._file_name

    @property
    def dimension(self):
        return self._dimension

    @property
    def extents(self):
        return self._extents

    @property
    def support(self):
        return self._support

    @property
    def strength(self):
        return self._strength

    def set_field(self, name, field):
        """
        Sets the Field `name` to `field`.
        """
        if field.extents != self.extents:
            raise ValueError("Field extents %s differ from potential extents %s!" % (field.extents, self.extents))
        self._fields[name] = field
        return self._fields[name]

    def get_field(self, name):
        """
        Gets the Field with the given `name`.
        If such a field does not exist, a new, empty field is added.
        :rtype: Field
        """
        if name not in self._fields:
            self._fields[name] = Field(self.extents)
        return self._fields[name]

    @property
    def field(self):
        """
        Gets the unique field present in this potential.
        :raises: LookupError, if potential does not contain exactly one field.
        :rtype: Field
        """
        if len(self._fields) != 1:
            raise LookupError("Can only use field property for Potentials that contain a single field.")
        return next(self._fields.itervalues())

    @property
    def fields(self):
        """
        Gets and iterator over the names of all fields that belong to the potential.
        :return:
        """
        return self._fields.viewkeys()


class LazyPotential(Potential):
    """
    This is an implementation of `Potential` that assumes that the potential data is given in a file, and only
    loads the actual data if there is an attempt to access it.
    """
    def __init__(self, source_file):
        if isinstance(source_file, str):
            source_file = open(source_file, "rb")
        super(LazyPotential, self).__init__(_read_pot_config(source_file))
        self._source_file = source_file
        self._loaded = False
        self._file_name = self._source_file.name

    def set_field(self, name, field):
        """
        Sets the Field `name` to `field`.
        """
        self._ensure_loaded()
        super(LazyPotential, self).set_field(name, field)

    def get_field(self, name):
        """
        Gets the Field with the given `name`.
        If such a field does not exist, a new, empty field is added.
        :rtype: Field
        """
        self._ensure_loaded()
        return super(LazyPotential, self).get_field(name)

    @property
    def field(self):
        """:rtype: Field"""
        self._ensure_loaded()
        return super(LazyPotential, self).field

    def _ensure_loaded(self):
        """
        If the potential data has not been loaded yet, this function performs loading of
        the data, otherwise it does nothing.
        """
        if self._loaded:
            return

        _logger.info("Loading potential data from file %r.", self._source_file)
        cfg = _read_pot_config(self._source_file)

        # we need to pretend that we are loaded here, so _read_fields does not start an infinite recursion when
        # accessing the fields to write to.
        self._loaded = True
        _read_fields(self._source_file, self, cfg.grids)
