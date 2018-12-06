# -*- coding: utf-8 -*-
"""
Created on Fri Jan 09 17:30:14 2015

@author: Erik
"""

import numpy as np
from collections import Iterable


def read_int(f, count=1):
    """
    Reads integers from a file. The integers are processed as an 8 byte unsigned datatype,
    corresponding to the way integers are saved from the c++ side.
    See `read_array` for more info.
    """
    return read_array(f, np.uint64, count)


def read_float(f, count=1):
    """
    Reads floats from a file. Always reads as 64 bit doubles.
    See `read_array` for more info.
    """
    return read_array(f, np.float64, count)


def read_array(file_, dtype, count=1):
    """
    Reads an array from a file. The data type can be any numpy dtype.
    
    :param file|str file_: Source file from which to read.
    :param dtype: Type (as numpy data type) of the data which to read.
    :param int|Iterable count: The number of elements to read. Set to -1 to read until eof. \
        Can also be a tuple, in which case the read data will be reshaped to match it.
    :return: number|np.ndarray: If count==1, returns a single element, otherwise \
        an array of elements of type `dtype`. If count was specified as a tuple, \
        the shape of the array will be `count`.
    :raises IOError: if less data could be read than specified by count.
    """
    if isinstance(count, Iterable):
        count = tuple(count)  # ensure that we iterate the iterable only once!
        total = np.prod(count)
        result = read_array(file_, dtype, total)
        return result.reshape(count)

    result = np.fromfile(file_, dtype=dtype, count=count)
    if len(result) != count and count != -1:
        raise IOError("Expected to read {} entries of type {} but got only {}".format(count, dtype.__name__,
                                                                                      len(result)))
    if count == 1:
        return result[0]
    return result


def read_grid(file_, count=1):
    """ 
    Loads a grid saved from c++ from a file. This is basically an array with a shape and data type
    assigned to it.
    
    :param BinaryIO file_: File object from which the grid is loaded. Has to be open in binary mode.
    :param int count: Number of grids to read.
    :return np.ndarray: A numpy array representing the grid data in correct shape, or a list of numpy arrays \
        in case of `count != 1`.
    :raises IOError: If the file does not contain a grid at the current position, or any form of data corruption could \
        be detected.
    :raises NotImplementedError: If the data type in the grid is not one of `int64`, `uint64`, `float64`, \
        `float32`, `uint32`.
    """
    data = [_read_single_grid(file_) for _ in range(count)]
    if count == 1:
        return data[0]
    else:
        return data


def _read_single_grid(file_):
    """
    Reads a single data grid from `file_`.
    """
    # read header
    if file_.read(1) != 'g':
        raise IOError("The file %s does not contain a grid at the current reading position." % file_)

    # read shape
    dim = read_int(file_)
    size = np.fromfile(file_, dtype=np.uint64, count=dim)
    if len(size) != dim:
        raise IOError("Corrupt grid data, trying to read {} dimensions but got {}".format(dim, len(size)))

    data_type = _read_dtype(file_)
    num_elements = read_int(file_)

    if num_elements != np.prod(size):
        raise IOError("Number of elements {} are incompatible with specified shape {}".format(num_elements, size))

    return read_array(file_, data_type, size)


def _read_dtype(f):
    typestr = ''
    while True:
        c = f.read(1)  # type: chr
        if c != '\0':
            typestr += c
            if not c.isalnum():
                raise IOError("Invalid type string {}".format(typestr))
        else:
            break
    if typestr == 'd':
        data_type = np.float64
    elif typestr == 'f':
        data_type = np.float32
    elif typestr == 'm':
        data_type = np.uint64
    elif typestr == 'l':
        data_type = np.int64
    elif typestr == 'j':
        data_type = np.uint32
    else:
        raise NotImplementedError("unsupported grid type %r" % typestr)
    return data_type


def is_potential_file(filename):
    """
    Tests whether the given file is a potential file.
    
    :param str filename: Name of the file to check.
    :return: Whether the file is a potential file (or at least, the file starts with a potential).
    :rtype: bool
    """
    with open(filename, "rb") as candidate_file:
        return candidate_file.read(4) == "bpot"


def is_velocity_histogram(filename):
    """
    Tests whether the given file is a velocity histogram file.
    
    :param str filename: Name of the file to check.
    :return: Whether the file is a velocity histogram file.
    :rtype: bool
    """
    with open(filename, "rb") as file:
        return file.read(4) == "velh"


def is_velocity_transitions(filename):
    """
    Tests whether the given file is a velocity histogram file.
    
    :param str filename: Name of the file to check.
    :return: Whether the file is a velocity histogram file.
    :rtype: bool
    """
    with open(filename, "rb") as file:
        return file.read(4) == "velt"


