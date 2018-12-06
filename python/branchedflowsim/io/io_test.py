from .read_data import *
from .write_data import *
from ..test_utils import *

########################################################################################################################
#                                           INT
########################################################################################################################
def test_int_round_trip(file_):
    write_int(file_, 5)
    write_int(file_, [5, 4, 6])

    file_.seek(0)
    assert read_int(file_) == 5

    assert read_int(file_) == 5
    assert list(read_int(file_, 2)) == [4, 6]


def test_int_read_too_much(file_):
    write_int(file_, 5)
    file_.seek(0)
    with pytest.raises(IOError):
        read_int(file_, 5)


def test_int_read_tuple(file_):
    reference = np.array([[1, 2], [3, 4]], dtype=np.int64)
    write_int(file_, reference.flatten())

    # check that reading a single int as a shaped object works.
    file_.seek(0)
    assert read_int(file_, (1, 1)) == pytest.approx(np.ones((1, 1)))

    file_.seek(0)
    assert read_int(file_, (2, 2)) == pytest.approx(reference)

    # TODO what about tuples with unspecified shapes [e.g. (5, -1)]?


def test_write_negative_int(file_):
    with pytest.raises(ValueError):
        write_int(file_, -5)


def test_write_float_as_int(file_):
    with pytest.raises(TypeError):
        write_int(file_, 1.5)


def test_write_int_wrong_shape(file_):
    write_int(file_, 1, 1)
    with pytest.raises(ValueError):
        write_int(file_, [1, 2, 3], 5)


########################################################################################################################
#                                           FLOAT
########################################################################################################################
def test_float_round_trip(file_):
    write_float(file_, 5.8)
    write_float(file_, [5.1, 4.7, 6])

    file_.seek(0)
    assert read_float(file_) == 5.8

    assert read_float(file_) == 5.1
    assert list(read_float(file_, 2)) == [4.7, 6]


def test_float_read_too_much(file_):
    write_float(file_, 5)
    file_.seek(0)
    with pytest.raises(IOError):
        read_float(file_, 5)


def test_float_read_tuple(file_):
    reference = np.array([[1, 2], [3, 4]], dtype=np.float64)
    write_float(file_, reference.flatten())

    # check that reading a single int as a shaped object works.
    file_.seek(0)
    assert read_float(file_, (1, 1)) == pytest.approx(np.ones((1, 1)))

    file_.seek(0)
    assert read_float(file_, (2, 2)) == pytest.approx(reference)


def test_float_write_super_precision(file_):
    with pytest.raises(TypeError):
        write_float(file_, np.array([5], np.float128))


def test_write_float_wrong_shape(file_):
    write_float(file_, 1.0, 1)
    with pytest.raises(ValueError):
        write_float(file_, [1.0, 2.0, 3.0], 5)


########################################################################################################################
#                                           GRID
########################################################################################################################
@pytest.mark.parametrize("float_type", [np.float64, np.float32])
def test_grid_round_trip_float(float_type, file_):
    array = np.random.random((5, 17)).astype(float_type)
    write_grid(file_, array)

    file_.seek(0)
    loaded = read_grid(file_)

    assert loaded.dtype == float_type
    assert np.all(loaded == array)


@pytest.mark.parametrize("int_type", [np.int64, np.uint64])
def test_grid_round_trip_int(int_type, file_):
    array = np.random.randint(0, 10, size=(5, 17)).astype(int_type)
    write_grid(file_, array)

    file_.seek(0)
    loaded = read_grid(file_)

    assert loaded.dtype == int_type
    assert np.all(loaded == array)


def test_write_grid_unsupported_dtype(file_):
    array = np.random.randint(0, 10, size=(5, 17)).astype(np.complex)
    with pytest.raises(TypeError):
        write_grid(file_, array)


def test_read_grid_unsupported_dtype(file_):
    file_.write('g')
    write_int(file_, 1)
    write_int(file_, 1)
    file_.write('wrong\0')

    file_.seek(0)
    with pytest.raises(NotImplementedError):
        read_grid(file_)


def test_read_grid_wrong_pos(file_):
    array = np.random.randint(0, 10, size=(5, 17))
    write_grid(file_, array)

    file_.seek(1)
    with pytest.raises(IOError):
        loaded = read_grid(file_)


def test_read_corrupt_data():
    import tempfile

    with tempfile.TemporaryFile() as file_:
        array = np.random.randint(0, 10, size=(5, 17))
        write_grid(file_, array)
        file_.seek(1)
        write_int(file_, 5000)
        file_.seek(0)
        with pytest.raises(IOError):
            loaded = read_grid(file_)

    with tempfile.TemporaryFile() as file_:
        array = np.ones((5, 17)) * 1.54
        write_grid(file_, array)
        file_.seek(1)
        write_int(file_, 14)
        file_.seek(0)
        with pytest.raises(IOError):
            loaded = read_grid(file_)


def test_read_missing_data(file_):
    array = np.random.randint(0, 10, size=(5, 17))
    write_grid(file_, array)
    file_.seek(9)
    write_int(file_, 4)
    file_.seek(0)
    # number of elements incompatible with shape
    with pytest.raises(IOError):
        loaded = read_grid(file_)

    # fake number of elements too, then there should be no error
    file_.seek(27)
    write_int(file_, 68)
    file_.seek(0)
    loaded = read_grid(file_)
    assert loaded.shape == (4, 17)

    # now promise more than there is
    file_.seek(9)
    write_int(file_, 40)
    file_.seek(27)
    write_int(file_, 680)
    file_.seek(0)

    with pytest.raises(IOError):
        loaded = read_grid(file_)
