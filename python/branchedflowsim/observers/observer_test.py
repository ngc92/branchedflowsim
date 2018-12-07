import pytest
from observer import Observer


def test_CmdLineBuilder_arguments_test():
    class MonodromyFreeObserver(Observer):
        _ARGUMENTS_ = []
        _NAME_ = "NAME"

    with pytest.raises(TypeError):
        MonodromyFreeObserver()
