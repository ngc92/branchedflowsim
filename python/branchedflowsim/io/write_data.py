# -*- coding: utf-8 -*-
"""
Created on Fri Jan 09 17:30:14 2016

@author: Erik
"""

import numpy as np
import numbers


def write_int(f, value, shape=-1):
    """
    Write a single integer, or a collection of integers, to file f.
    The intent of this function is to be used to write low amounts of metadata into a file, and thus
    is optimized for safety and not for speed.
    
    :param BinaryIO f: File in which to write the integer.
    :param int|typing.Sequence[int] value: Value to write the file. Either a single integer, or
            something that can be converted to an array of ints.
    :param int|tuple shape: Expected shape of `value`. Causes an error to be raised if value is not
            shaped as specified here. Set to -1 (default) to disable checking.
    :raises ValueError: If `value` contains negative integers.
            TypeError: If `value` can not safely be converted to an array of unsigned ints.
            ValueError: If `value.shape` is different from `shape`.
    """

    ar = np.asarray(value)

    if not verify_shape(ar, shape):
        raise ValueError("Got data with shape {} but expected {}".format(ar.shape, shape))

    # safely convert to unsigned values
    try:
        converted = ar.astype(np.uint64, casting="safe")
    except TypeError:
        signed_data = ar.astype(np.int64, casting="safe")
        if np.min(signed_data) >= 0:
            converted = signed_data.astype(np.uint64)
        else:
            raise ValueError("Trying to save negative integer")

    converted.tofile(f)


def write_float(f, value, shape=-1):
    """
    Write a single float, or a collection of floats, to file f.
    The intent of this function is to be used to write low amounts of metadata into a file, and thus
    is optimized for safety and not for speed. It writes everything as 64 bit doubles.
    
    :param BinaryIO f: File in which to write the data.
    :param float|typing.Sequence[float] value: Value to write the file. Either a single float, or \
        something that can be converted to an array of floats.
    :param int|tuple shape: Expected shape of `value`. Causes an error to be raised if value is not \
        shaped as specified here. Set to -1 (default) to disable checking.
    :raises TypeError: If `value` cannot safely be converted to an array of floats. 
    :raises ValueError: If `value.shape` is different from `shape`.
    """

    ar = np.asarray(value)
    if not verify_shape(ar, shape):
        raise ValueError("Got data with shape {} but expected {}".format(ar.shape, shape))
    ar.astype(np.float64, casting="safe").tofile(f)


def verify_shape(value, shape):
    """
    Checks that `value` fulfills the shape specification given by `shape`.
    """
    if shape == -1:
        return True
    else:
        # ensure tuple
        if isinstance(shape, numbers.Integral):
            shape = (shape,)

        # special case for scalar
        if value.shape == () and shape == (1,):
            return True

        return value.shape == shape


def write_grid(f, grid):
    """
    writes a grid (array) so that it can be read from c++.
    
    :param BinaryIO f: A file opened for binary writing.
    :param np.ndarray grid: An array that is to be saved.
    :return:
    """

    # read header
    f.write('g')
    write_int(f, len(grid.shape))
    np.array(grid.shape).astype(np.uint64).tofile(f)

    # write data type
    if grid.dtype == np.dtype(np.float64):
        f.write('d\0')
    elif grid.dtype == np.dtype(np.float32):
        f.write('f\0')
    elif grid.dtype == np.dtype(np.uint64):
        f.write('m\0')
    elif grid.dtype == np.dtype(np.uint32):
        f.write('j\0')
    elif grid.dtype == np.dtype(np.int64):
        f.write('l\0')
    else:
        raise TypeError("unsupported grid type {}".format(grid.dtype))

    # write container
    write_int(f, grid.size)
    np.asarray(grid, order="C").tofile(f)
