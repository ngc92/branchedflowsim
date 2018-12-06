# tests the results file loading code
# this only tests the python layer, so if we change the c++ code, these tests won't fail.
# for this we need to figure out something smarter

import tempfile

import pytest

from branchedflowsim.results import *
from branchedflowsim.io.write_data import *


def _verify_equality(object_, dictionary, indices):
    for ind in indices:
        if isinstance(ind, tuple):
            name = ind[0]
            f = ind[1]
            assert f(getattr(object_, name)) == f(dictionary[name])
        else:
            assert getattr(object_, ind) == dictionary[ind]


def _verify_array_equal(a, b):
    a = np.asarray(a)
    b = np.asarray(b)
    assert a.shape == b.shape
    assert np.all(a == b)


class ResultFileIO(object):
    __TYPE__ = None
    __REDUCE__ = True

    @classmethod
    def load(cls, source_file):
        source_file.seek(0)
        return cls.__TYPE__(source_file)

    @classmethod
    def from_dict(cls):
        return cls.__TYPE__(cls.source_dict())

    @staticmethod
    def write(target_file):
        raise NotImplementedError()

    @classmethod
    def reduced(cls):
        raise NotImplementedError()

    @staticmethod
    def source_dict():
        raise NotImplementedError()

    @staticmethod
    def verify(data, reference):
        raise NotImplementedError()


class TestVelocityTransitionsIO(ResultFileIO):
    __TYPE__ = VelocityTransitions

    @staticmethod
    def write(target_file):
        target_file.seek(0)
        target_file.write('velt002\n')
        write_int(target_file, 6)  # 6 bins
        write_int(target_file, 2)  # 2 dimensions
        write_float(target_file, 0.2)  # time interval
        write_float(target_file, [1] * 6)  # velocities
        write_grid(target_file, np.ones((6, 6)))

    @staticmethod
    def source_dict():
        return {
            "num_bins": 6,
            "dimensions": 2,
            "time_interval": 0.2,
            "velocities": [1] * 6,
            "counts": np.ones((6, 6))
        }

    @classmethod
    def reduced(cls):
        old = cls.source_dict()
        old.update({"counts": 2*np.ones((6, 6))})
        return old

    @staticmethod
    def verify(loaded, reference):
        _verify_equality(loaded, reference, ["dimensions", "num_bins", "time_interval", ("velocities", list)])
        _verify_array_equal(loaded.counts, reference["counts"])


class TestVelocityHistogramsIO(ResultFileIO):
    __TYPE__ = VelocityHistograms

    @staticmethod
    def write(target_file):
        target_file.seek(0)
        target_file.write('velh001\n')
        write_int(target_file, 25)  # 25 hists
        write_int(target_file, 50)  # 50 bins
        write_int(target_file, 2)  # 2 dimensions
        write_float(target_file, [0.5] * 25)  # times
        write_float(target_file, [1] * 50)  # velocities
        for i in range(25):
            write_grid(target_file, np.ones((50,)))

    @staticmethod
    def source_dict():
        return {
            "num_bins": 50,
            "num_hists": 25,
            "dimensions": 2,
            "times": [0.5] * 25,
            "velocities": [1] * 50,
            "counts": np.ones((25, 50))
        }

    @classmethod
    def reduced(cls):
        old = cls.source_dict()
        old.update({"counts": 2 * np.ones((25, 50))})
        return old

    @staticmethod
    def verify(loaded, reference):
        _verify_equality(loaded, reference, ["dimensions", "num_bins", "num_hists",
                                             ("velocities", list), ("times", list), ("counts", len)])
        _verify_array_equal(loaded.counts, reference["counts"])


class TestAngleHistogramsIO(ResultFileIO):
    __TYPE__ = AngleHistograms

    @staticmethod
    def write(target_file):
        target_file.write('angh001\n')
        write_int(target_file, 25)  # 25 hists
        write_int(target_file, 50)  # 50 bins
        write_float(target_file, [0.5] * 25)  # times
        write_float(target_file, [0.5] * 50)  # angles
        write_float(target_file, [0.8] * 25)  # sum
        write_float(target_file, [1.5] * 25)  # sumsq
        write_int(target_file, [1] * 25 * 50)

    @staticmethod
    def source_dict():
        return {
            "num_bins": 50,
            "num_hists": 25,
            "angles": [0.5] * 50,
            "sum_angles": [0.8] * 25,
            "sum_angles_squared": [1.5] * 25,
            "times": [0.5] * 25,
            "counts": np.ones((25, 50), dtype=int)
        }

    @classmethod
    def reduced(cls):
        old = cls.source_dict()
        old.update(
            {"counts": 2 * np.ones((25, 50), dtype=int),
             "sum_angles": [2*0.8] * 25,
             "sum_angles_squared": [2*1.5] * 25
             }
        )
        return old

    @staticmethod
    def verify(loaded, reference):
        _verify_equality(loaded, reference, ["num_bins", "num_hists", ("angles", list), ("sum_angles", list),
                                             ("sum_angles_squared", list), ("times", list)])
        _verify_array_equal(loaded.counts, reference["counts"])


class TestTrajectoriesIO(ResultFileIO):
    __TYPE__ = Trajectories
    __REDUCE__ = False

    @staticmethod
    def write(target_file):
        from branchedflowsim.results.trajectories import trajectory_sample_type
        tst = trajectory_sample_type(2)
        target_file.write('traj001\n')
        write_int(target_file, 2)  # dimension
        write_int(target_file, 50)  # counter
        write_int(target_file, 50)  # num_samples
        tdata = np.ones(dtype=tst, shape=(50,))
        tdata.tofile(target_file)

    @staticmethod
    def source_dict():
        from branchedflowsim.results.trajectories import trajectory_sample_type
        tst = trajectory_sample_type(2)
        trajectory_data = np.ones(dtype=tst, shape=(50,))
        return {
            "dimension": 2,
            "max_index": 50,
            "trajectories": trajectory_data
        }

    @classmethod
    def reduced(cls):
        from branchedflowsim.results.trajectories import trajectory_sample_type
        tst = trajectory_sample_type(2)
        trajectory_data = np.ones(dtype=tst, shape=(100,))

        old = cls.source_dict()
        old.update(
            {"max_index": 100,
             "trajectories": trajectory_data
             })
        return old

    @staticmethod
    def verify(loaded, reference):
        trajectory_data = reference["trajectories"]

        _verify_equality(loaded, reference, ["dimension", "max_index", ("trajectories", len)])

        _verify_array_equal(loaded.trajectories, trajectory_data)
        _verify_array_equal(loaded.positions, trajectory_data["position"])
        _verify_array_equal(loaded.velocities, trajectory_data["velocity"])
        _verify_array_equal(loaded.times, trajectory_data["time"])


class TestCausticsIO(ResultFileIO):
    __TYPE__ = Caustics

    @staticmethod
    def write(target_file):
        from branchedflowsim.results.caustics import caustic_type
        tst = caustic_type(2)
        target_file.write("caus001\n")
        write_int(target_file, 50)  # ray count
        write_int(target_file, 2)  # dimension
        write_int(target_file, 50)  # caustic count
        tdata = np.ones(dtype=tst, shape=(50,))
        tdata.tofile(target_file)

    @staticmethod
    def source_dict():
        from branchedflowsim.results.caustics import caustic_type
        tst = caustic_type(2)
        caustic_data = np.ones(dtype=tst, shape=(50,))

        return {
            "dimension": 2,
            "raycount": 50,
            "caustics": caustic_data
        }

    @classmethod
    def reduced(cls):
        from branchedflowsim.results.caustics import caustic_type
        tst = caustic_type(2)
        caustic_data = np.ones(dtype=tst, shape=(100,))

        old = cls.source_dict()
        old.update(
            {"raycount": 100,
             "caustics": caustic_data
             }
        )
        return old

    @staticmethod
    def verify(loaded, reference):
        caustic_data = reference["caustics"]

        _verify_equality(loaded, reference, ["dimension", "raycount", ("caustics", len)])
        _verify_array_equal(loaded.caustics, caustic_data)
        _verify_array_equal(loaded.positions, caustic_data["position"])
        _verify_array_equal(loaded.times, caustic_data["time"])


ResultFileTypes = [TestVelocityTransitionsIO, TestVelocityHistogramsIO, TestAngleHistogramsIO, TestTrajectoriesIO,
                   TestCausticsIO]


@pytest.mark.parametrize("setup", ResultFileTypes)
def test_from_dict(setup):
    loaded = setup.from_dict()
    setup.verify(loaded, setup.source_dict())


@pytest.mark.parametrize("setup", ResultFileTypes)
def test_loading(setup):
    with tempfile.TemporaryFile() as file_:
        setup.write(file_)
        setup.verify(setup.load(file_), setup.source_dict())


@pytest.mark.parametrize("setup", ResultFileTypes)
def test_saving(setup):
    with tempfile.TemporaryFile() as file_:
        setup.from_dict().to_file(file_)
        setup.verify(setup.load(file_), setup.source_dict())


@pytest.mark.parametrize("setup", ResultFileTypes)
def test_reduction(setup):
    with tempfile.TemporaryFile():
        a = setup.from_dict()
        if setup.__REDUCE__:
            a.reduce(setup.from_dict())
            setup.verify(a, setup.reduced())
        else:
            with pytest.raises(AssertionError):
                a.reduce(setup.from_dict())
