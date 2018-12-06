from .data_spec import *
from ..test_utils import *


def test_new_checking():
    with pytest.raises(TypeError):
        d = DataSpec("test", "unknown_type", 5)

    with pytest.raises(TypeError):
        d = DataSpec("test", float, 5, reduction=5)


def test_valid_dtypes():
    assert DataSpec.is_valid_dtype(int) is True
    assert DataSpec.is_valid_dtype(np.float32) is True
    assert DataSpec.is_valid_dtype("grid") is True
    assert DataSpec.is_valid_dtype("int64") is True
    assert DataSpec.is_valid_dtype("SPAM") is False

    # compound numpy type
    cnt = np.dtype([("data", np.uint64), ("position", np.double, (5,))])
    assert DataSpec.is_valid_dtype(cnt) is True


@pytest.mark.parametrize("spec, result",
                         [(5, 5),
                          ((6, 1), (6, 1)),
                          ([6, 1], (6, 1))])
def test_resolve_count(spec, result):
    assert DataSpec.resolve_count(spec, {}) == result


@pytest.mark.parametrize("spec, result",
                         [("count", 7),
                          (("count", "other", 5), (7, 6, 5))])
def test_resolve_count_with_placeholders(spec, result):
    assert DataSpec.resolve_count(spec, {"count": 7, "other": 6, 6: "SPAM"}) == result


def test_resolve_count_checking():
    with pytest.raises(KeyError):
        DataSpec.resolve_count("count", {})

    with pytest.raises(TypeError):
        DataSpec.resolve_count(None, {})


def test_resolve_dtype():
    assert DataSpec.resolve_dtype(np.float64, {}) == np.float64
    type_fun = lambda data: data["dtype"]
    assert DataSpec.resolve_dtype(type_fun, {"dtype": np.int32}) == np.int32


def test_resolve_dtype_checking():
    type_fun = lambda data: data["dtype"]
    with pytest.raises(TypeError):
        DataSpec.resolve_dtype(type_fun, {"dtype": 5})

    with pytest.raises(KeyError):
        DataSpec.resolve_dtype(type_fun, {})


def test_read_spec(file_):
    type_fn = lambda x: int
    spec = DataSpec("data", type_fn, "amount")
    write_int(file_, [0, 1, 2, 3, 4])
    file_.seek(0)

    data = {"amount": 5}
    spec.read(file_, data)
    assert list(data["data"]) == [0, 1, 2, 3, 4]


def test_write_spec(file_):
    type_fn = lambda x: int
    spec = DataSpec("data", type_fn, "amount")

    data = {"amount": 5, "data": [0, 1, 2, 3, 4]}
    spec.write(file_, data)

    file_.seek(0)
    assert list(read_int(file_, 5)) == [0, 1, 2, 3, 4]


def test_reduction_add():
    # works with single numbers and np arrays, and treats lists as arrays
    assert Reductions.add(5, 8) == 13
    assert list(Reductions.add(np.array([1, 2, 3]), np.array([3, 2, 1]))) == [4, 4, 4]
    assert list(Reductions.add([1, 2, 3], [3, 2, 1])) == [4, 4, 4]


def test_reduction_equal():
    assert Reductions.equal(5, 5) == 5
    with pytest.raises(AssertionError):
        Reductions.equal(5, 6)

    assert Reductions.equal(np.ones(5), np.ones(5)) == pytest.approx(np.ones(5))


def test_reduction_fail():
    with pytest.raises(AssertionError):
        Reductions.fail(5, 4)


def test_reduction_fail():
    with pytest.raises(AssertionError):
        Reductions.fail(5, 4)


def test_reduction_concat():
    a1 = [1, 2, 3]
    a2 = [4, 5, 6]
    assert list(Reductions.concat(a1, a2)) == [1, 2, 3, 4, 5, 6]
    assert list(Reductions.concat(np.array(a1), np.array(a2))) == [1, 2, 3, 4, 5, 6]
