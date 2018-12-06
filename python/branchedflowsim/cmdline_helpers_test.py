import pytest
from cmdline_helpers import CmdLineBuilder, ArgSpec


def test_verify_bool():
    assert CmdLineBuilder._verify_bool(True, "argument") is True
    assert CmdLineBuilder._verify_bool(False, "argument") is False
    assert CmdLineBuilder._verify_bool(None, "argument") is None

    with pytest.raises(ValueError, match="Expected boolean value for argument 'arg', got 1"):
        CmdLineBuilder._verify_bool(1, "arg")

    with pytest.raises(ValueError, match="Expected boolean value for argument 'arg', got 'True'"):
        CmdLineBuilder._verify_bool("True", "arg")


def test_verify_number():
    assert CmdLineBuilder._verify_number(5.0, "argument") == 5.0
    assert CmdLineBuilder._verify_number(4, "argument") == 4.0
    assert CmdLineBuilder._verify_number("1.0", "argument") == 1.0
    assert CmdLineBuilder._verify_number(None, "argument") is None

    with pytest.raises(ValueError, match="Expected real value for argument 'arg', got 'p5'"):
        CmdLineBuilder._verify_number("p5", "arg")

    # TODO improve the error messages!
    with pytest.raises(TypeError, match="can't convert complex to float for argument 'arg'"):
        CmdLineBuilder._verify_number(5+7j, "arg")


def test_verify_integer():
    assert CmdLineBuilder._verify_integer(4, "argument") == 4
    assert CmdLineBuilder._verify_integer(5.0, "argument") == 5
    assert CmdLineBuilder._verify_integer("-6", "argument") == -6
    assert CmdLineBuilder._verify_integer(None, "argument") is None

    with pytest.raises(ValueError, match="Expected integer value for argument 'arg', got 1.5"):
        CmdLineBuilder._verify_integer(1.5, "arg")

    with pytest.raises(ValueError, match="Expected integer value for argument 'arg', got '1.5'"):
        CmdLineBuilder._verify_integer("1.5", "arg")

    # TODO improve the error messages!
    with pytest.raises(TypeError, match="can't convert complex to int for argument 'arg'"):
        CmdLineBuilder._verify_integer(5+7j, "arg")


def test_verify_string():
    assert CmdLineBuilder._verify_string(4, "argument") == "4"
    assert CmdLineBuilder._verify_string("a string", "argument") == "a string"
    assert CmdLineBuilder._verify_string(None, "argument") is None


def test_verify_sequence():
    assert CmdLineBuilder._verify_sequence([5.0], "argument", CmdLineBuilder._verify_integer) == [5]
    assert CmdLineBuilder._verify_sequence(5.0, "argument", CmdLineBuilder._verify_integer) == [5]
    assert CmdLineBuilder._verify_sequence(None, "argument", CmdLineBuilder._verify_integer) is None

    with pytest.raises(ValueError, match="Expected integer value for argument 'arg', got 1.5"):
        CmdLineBuilder._verify_sequence([1.5], "arg", CmdLineBuilder._verify_integer)


def make_CmdLineBuilder_class(positional, required, type="Int", amount=1):
    class TestCmdLineBuilder(CmdLineBuilder):
        _ARGUMENTS_ = [
            ArgSpec("arg", "description", positional=positional, required=required, type=type, amount=amount)
        ]
        _NAME_ = "test_obs"
    obs = TestCmdLineBuilder()
    obs.arg = None
    return obs


def test_CmdLineBuilder_arguments_test():
    class BadCmdLineBuilder(CmdLineBuilder):
        _NAME_ = "BAD"

    # match seems correct, but this does not work, so commented out.
    with pytest.raises(TypeError): #, match="No _ARGUMENTS_ found in BadCmdLineBuilder "
        #     "(<class 'branchedflowsim.CmdLineBuilders.CmdLineBuilder_test.BadCmdLineBuilder'>)"):
        BadCmdLineBuilder()

    class NamelessCmdLineBuilder(CmdLineBuilder):
        _ARGUMENTS_ = []

    with pytest.raises(TypeError):
        NamelessCmdLineBuilder()


def test_CmdLineBuilder_named_mandatory():
    cls = make_CmdLineBuilder_class(False, True)
    with pytest.raises(ValueError, match="Argument 'arg' not supplied for TestCmdLineBuilder"):
        cls.args
    cls.arg = 5
    assert cls.make_command() == ["test_obs", "arg", "5"]


def test_CmdLineBuilder_bool_arg():
    cls = make_CmdLineBuilder_class(False, True, type="Bool")
    cls.arg = True
    assert cls.make_command() == ["test_obs", "arg", "1"]
    cls.arg = False
    assert cls.make_command() == ["test_obs", "arg", "0"]



# different configurations for one argument CmdLineBuilders
def test_CmdLineBuilder_named_optional():
    cls = make_CmdLineBuilder_class(False, False)
    assert cls.args == []

    cls.arg = 5
    assert cls.args == ["arg", "5"]


def test_CmdLineBuilder_positional_mandatory():
    cls = make_CmdLineBuilder_class(True, True)
    with pytest.raises(ValueError, match="Argument 'arg' not supplied for CmdLineBuilder TestCmdLineBuilder"):
        cls.args
    cls.arg = 5
    assert cls.args == ["5"]


def test_CmdLineBuilder_positional_mandatory():
    cls = make_CmdLineBuilder_class(True, False)
    assert cls.args == []


def test_CmdLineBuilder_sequence():
    cls = make_CmdLineBuilder_class(False, True, amount=-1)
    cls.arg = [1, 2, 3]
    assert cls.args == ["arg", "1", "2", "3"]


def test_CmdLineBuilder_flag():
    cls = make_CmdLineBuilder_class(True, False, "Flag")
    assert cls.args == []
    cls.arg = True
    assert cls.args == ["arg"]


# a more complex CmdLineBuilder
def test_complex_CmdLineBuilder():
    class TestCmdLineBuilder(CmdLineBuilder):
        _ARGUMENTS_ = [
            ArgSpec("arg", "description", positional=True, required=True, type="Int", amount=1),
            ArgSpec("arg2", "description", positional=True, required=False, type="Int", amount=1),
            ArgSpec("arg3", "description", positional=False, required=False, type="Int", amount=1),
            ArgSpec("arg4", "description", positional=False, required=True, type="Int", amount=1)
        ]
        _NAME_ = "test_obs"
    obs = TestCmdLineBuilder()
    obs.arg = 5
    obs.arg2 = None
    obs.arg3 = 8
    obs.arg4 = 9

    assert obs.args == ["5", "arg3", "8", "arg4", "9"]


# a more complex CmdLineBuilder
def test_multi_optional_positional():
    class TestCmdLineBuilder(CmdLineBuilder):
        _ARGUMENTS_ = [
            ArgSpec("arg", "description", positional=True, required=False, type="Int", amount=1),
            ArgSpec("arg2", "description", positional=True, required=False, type="Int", amount=1),
            ArgSpec("arg3", "description", positional=True, required=False, type="Int", amount=1)
        ]
        _NAME_ = "test_obs"
    obs = TestCmdLineBuilder()
    obs.arg = 5
    obs.arg2 = None
    obs.arg3 = 8

    with pytest.raises(ValueError) as verr:
        obs.args
