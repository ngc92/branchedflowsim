import os


try:
    import unittest.mock as mock
except ImportError:
    import mock as mock
from ..test_utils import *
from .result_file import ResultFile
from . import DataSpec, write_int


class Dummy(ResultFile):
    _SPEC_ = ()
    _FILE_NAME_ = "default_name"

    def __init__(self, arg=None):
        super(Dummy, self).__init__(arg)


def test_spec_check():
    class Bad(ResultFile):
        pass

    with pytest.raises(ValueError):
        Bad()


###############################################################################
#                           Tests for init dispatch
###############################################################################
def test_init_from_file(file_):
    with mock.patch.object(Dummy, 'from_file') as ff:
        # open directly from file
        d = Dummy(file_)
        ff.assert_called_once_with(file_)


def mock_is_file(path):
    return path == "test_file"


@mock.patch("os.path.isfile", side_effect=mock_is_file)
def test_init_from_filename(_):
    # direct file name lookup
    with mock.patch.object(Dummy, 'from_file') as ff:
        # open directly from file
        d = Dummy("test_file")
        ff.assert_called_once_with("test_file")

    # using the default name
    with mock.patch.object(Dummy, 'from_file') as ff:
        # open directly from file
        d = Dummy("path")
        ff.assert_called_once_with(os.path.join("path", "default_name"))


def test_init_non_existing_file(monkeypatch):
    monkeypatch.delattr(Dummy, "_FILE_NAME_")
    with pytest.raises(IOError):
        d = Dummy("test_file")


def test_init_from_dict():
    with mock.patch.object(Dummy, "from_dict") as fd:
        # open directly from file
        data = {"a": 5, "b": 8}
        d = Dummy(data)
        fd.assert_called_once_with(data)


###############################################################################
#                   Test for from_file / from_dict
###############################################################################
def test_from_file_header_check(file_):
    d = Dummy()
    d._FILE_HEADER_ = "dummy"

    with pytest.raises(IOError):
        d.from_file(file_)

    d.from_dict = mock.MagicMock()

    file_.seek(0)
    file_.write("dummy")
    file_.seek(0)
    d.from_file(file_)

    d.from_dict.assert_called_once_with({})


def test_from_file(file_, monkeypatch):
    spec = (DataSpec("value", int, 1),)
    monkeypatch.setattr(Dummy, "_SPEC_", spec)

    write_int(file_, 5)
    file_.seek(0)
    with mock.patch.object(Dummy, "_from_file") as _from_file:  # type: mock.MagicMock
        with mock.patch.object(Dummy, "from_dict") as from_dict:
            d = Dummy()
            d.from_file(file_)

            _from_file.assert_called_once_with(file_,   {"value": 5})
            from_dict.assert_called_once_with({"value": 5})


def test_from_dict(monkeypatch):
    spec = (DataSpec("value", int, 1), DataSpec("spam", int, 1, is_attr=False))
    monkeypatch.setattr(Dummy, "_SPEC_", spec)

    data = {"value": 8, "spam": 10}
    with mock.patch.object(Dummy, "_from_dict") as _from_dict:  # type: mock.MagicMock
        d = Dummy()
        d.from_dict(data)
        _from_dict.assert_called_once_with(data)

        assert d.value == 8
        assert not hasattr(d, "spam")


def test_from_dict_type_check():
    d = Dummy()
    with pytest.raises(TypeError):
        d.from_dict("spam")


###############################################################################
#                   Test for to_file
###############################################################################
@pytest.fixture()
def results(monkeypatch):
    spec = (DataSpec("value", int, 1), DataSpec("spam", int, 1, is_attr=False))
    monkeypatch.setattr(Dummy, "_SPEC_", spec)

    data = {"value": 5, "spam": 10}
    return Dummy(data)


def test_write_header(file_):
    d = Dummy()
    d._FILE_HEADER_ = "dummy"
    d.to_file(file_)

    file_.seek(0)
    assert file_.read() == "dummy"


def test_to_file(file_, results):
    def to_file_check(data):
        assert data == {"value": 5}
        data["spam"] = 10

    with mock.patch.object(results, "_to_file", side_effect=to_file_check) as _to_file:  # type: mock.MagicMock
        results.to_file(file_)
        assert _to_file.call_count == 1


def test_to_file_errors_missing_data(file_, results):
    # missing data for non-spec value
    with pytest.raises(KeyError):
        results.to_file(file_)

    with pytest.raises(AttributeError):
        del results.value
        results.to_file(file_)


def test_to_file_errors_wrong_type(file_, results):
    # wrong data type for a spec
    results.value = 5.5
    with pytest.raises(TypeError):
        results.to_file(file_)


def test_to_file_errors_wrong_shape(file_, results):
    # wrong shape for a spec
    results.value = [1, 2, 3]
    with pytest.raises(ValueError):
        results.to_file(file_)


###############################################################################
#                   Test for reduce
###############################################################################
def test_reduce(monkeypatch):
    spec = (DataSpec("value", int, 1, reduction="add"), DataSpec("useless", int, 1, is_attr=False))
    monkeypatch.setattr(Dummy, "_SPEC_", spec)

    d1 = Dummy({"value": 5})
    d2 = Dummy({"value": 10})

    d3 = d1.reduce(d2)

    assert d3.value == 15


def test_reduce_errors():
    d = Dummy()

    with pytest.raises(TypeError):
        d.reduce("str")


def test_reduce_unary():
    d = Dummy()
    assert d.reduce(None) is d
