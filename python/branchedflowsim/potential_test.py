import pytest
import numpy as np
from .test_utils import *
from branchedflowsim.potential import _read_pot_config, _read_fields, Potential, Field, PotCfg


@pytest.fixture()
def potential_file(file_):
    from branchedflowsim.io import write_grid, write_float, write_int
    info = "123456789\n"
    grid_count = 3
    extents = [5, 10, 15]

    file_.write("bpot5")
    file_.write(str(len(info)+1)+"\n")
    file_.write(info)

    write_int(file_, 3)
    write_float(file_, [1.0, 2.0, 3.0])
    write_int(file_, extents)
    write_int(file_, 17)
    write_int(file_, 2)
    write_int(file_, grid_count)
    write_float(file_, 0.1)
    write_float(file_, 0.5)

    for i in range(0, grid_count):
        write_int(file_, 5)
        file_.write("abcde")
        write_int(file_, [i, 0, 0])
        data = np.zeros(extents)
        data[i] = 1
        write_grid(file_, data)

    file_.seek(0)
    return file_


def test_read_potential_config(potential_file):
    config = _read_pot_config(potential_file)

    assert config.dimension == 3
    assert config.support == pytest.approx([1.0, 2.0, 3.0])
    assert config.extents == (5, 10, 15)
    assert config.grids == 3
    assert config.strength == 0.5

    assert config.meta['seed'] == 17
    assert config.meta['corrlength'] == 0.1
    assert config.meta['version'] == 2
    assert config.meta['info'] == "123456789\n"


def test_read_fields(potential_file):
    _read_pot_config(potential_file)
    target_potential = mock.Mock()
    target_field = Field([5, 10, 15])
    target_potential.get_field = mock.Mock(return_value=target_field)
    target_potential.dimension = 3
    _read_fields(potential_file, target_potential, 3)

    for i in range(0, 3):
        data = np.zeros([5, 10, 15])
        data[i] = 1
        assert target_field.get_partial_derivative(i, 0, 0) == pytest.approx(data)


def test_write_potential_meta(file_):
    pot = Potential(PotCfg(dimension=2, support=[1.0, 2.0], extents=[10, 20], strength=1, grids=1,
                           meta={"seed": 10, "version": 2, "info": "test", "corrlength": 0.01}))

    pot.get_field("pot").set_partial_derivative(np.zeros((10, 20)), 1, 2)
    pot.to_file(file_)
    file_.seek(0)
    config = _read_pot_config(file_)
    assert config.extents == (10, 20)
    assert config.support == pytest.approx([1.0, 2.0])
    assert config.strength == 1.0
    assert config.grids == 1
    assert config.dimension == 2
    assert config.meta == pot._meta


def test_read_write_potential(file_):
    pot = Potential(PotCfg(dimension=2, support=[1.0, 2.0], extents=[10, 20], strength=1, grids=1,
                           meta={"seed": 10, "version": 2, "info": "test", "corrlength": 0.01}))

    pot.get_field("pot").set_partial_derivative(np.random.rand(10, 20), 1, 2)
    pot.get_field("pot").set_partial_derivative(np.random.rand(10, 20), 1, 1)
    pot.to_file(file_)
    file_.seek(0)
    rt = Potential.from_file(file_)

    assert rt.dimension == pot.dimension
    assert rt.strength == pot.strength
    assert rt.support == pytest.approx(pot.support)
    assert rt.extents == pot.extents
    assert rt.file_name == file_.name

    # trigger the lazy loading
    rt.field
    assert list(rt.fields) == list(pot.fields)

    assert rt.field.get_partial_derivative(1, 1) == pytest.approx(pot.field.get_partial_derivative(1, 1))
    assert rt.field.get_partial_derivative(1, 2) == pytest.approx(pot.field.get_partial_derivative(1, 2))
