import pytest
from collections import OrderedDict
from branchedflowsim import correlation


def test_correlation():
    c = correlation.Correlation("gauss", 0.5)
    assert c.kind == "gauss"
    assert c.scale == 0.5
    assert c.args == ["-c", "gauss", "-l", "0.5"]


def test_parametrized():
    c = correlation.Parametrized("gauss", 0.5, "param")
    assert c.kind == "gauss"
    assert c.scale == 0.5
    assert c.args == ["-c", "gauss", "param", "-l", "0.5"]


def test_iso_gauss():
    c = correlation.IsotropicGaussian(0.5)
    assert c.args == ["-c", "gauss", "-l", "0.5"]


def test_ani_gauss():
    c = correlation.AnisotropicGaussian(0.5, [1.0, 2.0, 2.0])
    assert c.scale_factors == (1.0, 2.0, 2.0)
    assert c.args == ["-c", "gauss", "1.0", "2.0", "2.0", "-l", "0.5"]


def test_script():
    c = correlation.CustomScript(0.5, "script", OrderedDict([("arg", 5), ("param", 8)]))
    assert c.script_file == "script"
    assert c.parameters == {"arg": 5, "param": 8}
    assert c.args == ["-c", "lua", "script", "arg", "5", "param", "8", "-l", "0.5"]


def test_trafo():
    c = correlation.Transformed(correlation.AnisotropicGaussian(0.5, [1, 2]), [1, 0, 0, 1])
    assert type(c.base_correlation) == correlation.AnisotropicGaussian
    assert c.transform == pytest.approx([1, 0, 0, 1])
    assert c.args == ["-c", "gauss", "1", "2", "--trafo", '"1 0 0 1"', "-l", "0.5"]


correlations = [
    correlation.Correlation("gauss", 0.5),
    correlation.Parametrized("gauss", 0.5, "param"),
    correlation.IsotropicGaussian(0.5),
    correlation.AnisotropicGaussian(0.5, [1.0, 2.0, 2.0]),
    correlation.CustomScript(0.5, "script", OrderedDict([("arg", 5), ("param", 8)])),
    correlation.Transformed(correlation.AnisotropicGaussian(0.5, [1, 2]), [1, 0, 0, 1])
]


@pytest.mark.parametrize("corr", correlations)
def test_as_dict_round_trip(corr):
    dict_rep = corr.as_dict

    reconstructed = correlation.Correlation.from_dict(dict_rep)

    assert corr == reconstructed
    assert corr.args == reconstructed.args
