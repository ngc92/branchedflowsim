import pytest
import mock
from branchedflowsim.medium import *
from branchedflowsim import correlation


@pytest.fixture()
def mock_generate():
    from branchedflowsim.potential import Potential, PotCfg

    def mocked(file_name, *args, **kwargs):
        cfg = PotCfg(2, [1, 1], [2, 2], 0, 1.0, {})
        p = Potential(cfg)
        p.to_file(open(file_name, "wb"))

    with mock.patch("branchedflowsim.potgen.generate", side_effect=mocked) as potgen:
        yield potgen


def test_medium_spec():
    ms = MediumSpec((25, 25), 2.0)
    assert ms.shape == (25, 25)
    assert ms.dimension == 2
    assert ms.support == pytest.approx(np.ones(2))
    assert ms.strength == 2.0
    assert str(ms) == "<MediumSpec on [0, 1]^2 (25x25)>"

    ms = MediumSpec([12, 16, 43], 2.5, [1.0, 2.0, 1.0])
    assert ms.shape == (12, 16, 43)
    assert ms.dimension == 3
    assert ms.support == pytest.approx([1.0, 2.0, 1.0])
    assert ms.strength == 2.5
    assert str(ms) == "<MediumSpec on [0, 1.0]x[0, 2.0]x[0, 1.0] (12x16x43)>"

    with pytest.raises(ValueError):
        ms = MediumSpec((12,), 2.5, [1.0, 2.0])


def test_scalar_potential_spec():
    cor = correlation.IsotropicGaussian(1.0)
    sp = ScalarPotentialSpec((25, 25), None, cor, 1.5)
    assert sp.shape == (25, 25)
    assert sp.dimension == 2
    assert sp.support == pytest.approx(np.ones(2))
    assert sp.strength == 1.5
    assert sp.correlation == cor

    sp = ScalarPotentialSpec(12, 3, cor, 1.5)
    assert sp.shape == (12, 12, 12)
    assert sp.dimension == 3
    assert sp.support == pytest.approx(np.ones(3))
    assert sp.strength == 1.5
    assert sp.correlation == cor

    with pytest.raises(ValueError):
        sp = ScalarPotentialSpec((25, 25), 3, cor, 1.5)

    sp = ScalarPotentialSpec((25, 25), None, cor, 1.5)
    assert sp.dimension == 2


def test_scalar_potential_generate(tmpdir, mock_generate):
    cor = correlation.IsotropicGaussian(1.0)
    sp = ScalarPotentialSpec((25, 25), None, cor, 1.5)
    target_file = str(tmpdir.join("target_file"))

    result = sp.create(target_file, 25)
    mock_generate.assert_called_once_with(target_file, 2, (25, 25), cor, 1.5, 25)
    assert result.file_name == target_file

    with mock.patch("branchedflowsim.potgen.generate") as potgen:
        result = sp.create(target_file, 25, {"option": "value"})
    potgen.assert_called_with(target_file, 2, (25, 25), cor, 1.5, 25, option="value")
    assert result.file_name == target_file


def test_generate_multiple(tmpdir, mock_generate):
    spec = ScalarPotentialSpec(16, 2, correlation.IsotropicGaussian(0.1), 1.0)
    spec.create = mock.Mock(side_effect=spec.create)
    for p_file in generate_multiple(spec, 5, options=None, work_dir=str(tmpdir)):
        spec.create.assert_called_once_with(p_file.file_name, options={})
        spec.create.reset_mock()
        assert p_file.file_name.startswith(str(tmpdir))

    assert mock_generate.call_count() == 5

