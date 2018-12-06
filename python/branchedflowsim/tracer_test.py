import pytest
import mock
import os
from branchedflowsim.observers import *
from branchedflowsim.config import trace_exe
from tracer import make_trace_command, trace, TraceResult, trace_multiple


def test_simple_tracing():
    cmd = make_trace_command("potential_file.pot", 0.5, 1000, "planar", "result_path", periodic=True, end_time=1.0)
    assert cmd == [trace_exe, "potential_file.pot", "-n", "1000",
                   "-s", "0.5", "-r", "result_path", "--end-time", "1.0", "--incoming", "planar", "--periodic"]


def test_tracing_w_incoming():
    cmd = make_trace_command("potential_file.pot", 0.5, 1000, "planar vx 1", "result_path", periodic=False)
    assert cmd == [trace_exe, "potential_file.pot", "-n", "1000",
                   "-s", "0.5", "-r", "result_path", "--end-time", "2.0", "--incoming", "planar", "vx", "1"]


def test_tracing_w_threads():
    cmd = make_trace_command("potential_file.pot", 0.5, 1000, "planar", "result_path", periodic=False, threads=12)
    assert cmd == [trace_exe, "potential_file.pot", "-n", "1000",
                   "-s", "0.5", "-r", "result_path", "--end-time", "2.0", "--incoming", "planar", "--threads", "12"]


def test_tracing_w_dynamics():
    cmd = make_trace_command("potential_file.pot", 0.5, 1000, "planar", "result_path", periodic=False, dynamics="sound")
    assert cmd == [trace_exe, "potential_file.pot", "-n", "1000",
                   "-s", "0.5", "-r", "result_path", "--end-time", "2.0", "--incoming", "planar", "--dynamics", "sound"]


def test_tracing_w_dynamics_list():
    cmd = make_trace_command("potential_file.pot", 0.5, 1000, "planar", "result_path", periodic=False,
                             dynamics="sound 5")
    assert cmd == [trace_exe, "potential_file.pot", "-n", "1000",
                   "-s", "0.5", "-r", "result_path", "--end-time", "2.0", "--incoming", "planar",
                   "--dynamics", "sound", "5"]


def test_tracing_w_observers():
    cstobs = Caustics(True)
    cmd = make_trace_command("potential_file.pot", 0.5, 1000, "planar", "result_path", periodic=False,
                             dynamics="sound 5", observers=[cstobs])
    assert cmd == [trace_exe, "potential_file.pot", "-n", "1000",
                   "-s", "0.5", "-r", "result_path", "--end-time", "2.0", "--incoming", "planar",
                   "--dynamics", "sound", "5", "--observers", "caustics", "1"]


def test_trace():
    cstobs = Caustics(True)
    cmd = make_trace_command("potential_file.pot", 0.5, 1000, "planar", "result_path", periodic=False,
                             dynamics="sound 5", observers=[cstobs])
    with mock.patch("subprocess.check_output") as check_output:
        result = trace("potential_file.pot", 0.5, 1000, "planar", "result_path", periodic=False, dynamics="sound 5",
                       observers=[cstobs])
        check_output.assert_called_once_with(cmd)

    assert result.basepath == "result_path"
    assert result.density is None
    assert set(result._loaders.viewkeys()) == {"caustics"}


def test_trace_result_empty():
    simple = TraceResult("basepath", [])

    assert simple.basepath == "basepath"
    assert len(simple._loaders) == 0
    assert len(simple._loaded_cache) == 0

    assert simple.density is None
    assert simple.caustics is None
    assert simple.angle_histogram is None
    assert simple.trajectories is None
    assert simple.velocity_histogram is None
    assert simple.velocity_transitions is None


def test_trace_result_simple():
    from branchedflowsim.observers import Caustics
    simple = TraceResult("basepath", [Caustics(True)])

    assert simple.basepath == "basepath"
    assert len(simple._loaders) == 1
    assert len(simple._loaded_cache) == 0

    assert simple.density is None
    assert simple.angle_histogram is None
    assert simple.trajectories is None
    assert simple.velocity_histogram is None
    assert simple.velocity_transitions is None

    with mock.patch("branchedflowsim.results.caustics.Caustics.__init__", return_value=None) as caustics:
        simple.caustics
        caustics.assert_called_once_with("basepath")


def test_trace_result_custom():
    from branchedflowsim.observers import Caustics
    custom = TraceResult("basepath", [Caustics(True, file_name="caustic_file.dat")])
    assert custom.caustics is None

    with mock.patch("branchedflowsim.results.caustics.Caustics.__init__", return_value=None) as caustics:
        custom.get_result("caustic_file.dat")
        caustics.assert_called_once_with(os.path.join("basepath", "caustic_file.dat"))


def test_trace_result_custom_multi():
    from branchedflowsim.observers import Caustics
    custom = TraceResult("basepath", [Caustics(True, file_name="caustic_file1.dat"),
                                      Caustics(True, file_name="caustic_file2.dat")])
    assert custom.caustics is None

    with mock.patch("branchedflowsim.results.caustics.Caustics.__init__", return_value=None) as caustics:
        custom.get_result("caustic_file1.dat")
        caustics.assert_called_once_with(os.path.join("basepath", "caustic_file1.dat"))

        custom.get_result("caustic_file2.dat")
        caustics.assert_called_with(os.path.join("basepath", "caustic_file2.dat"))


def test_multiple_tracings(tmpdir):
    potential = mock.Mock()
    potential.strength = 0.1
    potential.create = mock.Mock()

    with mock.patch("branchedflowsim.tracer.trace") as trace_mock:
        for result in trace_multiple(potential, 5, work_dir=str(tmpdir), potgen_options={"order": 1}, ray_count=1000,
                                     initial_condition="planar", observers=[]):
            assert trace_mock.call_args == mock.call(potential_file=mock.ANY, strength=0.1, path=mock.ANY,
                                                     ray_count=1000, initial_condition="planar", observers=[])
            assert result == trace_mock.return_value

        assert trace_mock.call_count == 5


def test_multiple_tracings_pot_order(tmpdir):
    from branchedflowsim.observers import Caustics
    potential = mock.Mock()
    potential.strength = 0.1

    with mock.patch("branchedflowsim.tracer.trace") as trace_mock,  \
            mock.patch("branchedflowsim.medium.generate_multiple") as gen_mock:
        for result in trace_multiple(potential, 5, work_dir=str(tmpdir), potgen_options={}, ray_count=1000,
                                     initial_condition="planar", observers=[]):
            pass
    gen_mock.assert_called_once_with(potential, 5, {"order": 1}, mock.ANY)

    with mock.patch("branchedflowsim.tracer.trace") as trace_mock, \
            mock.patch("branchedflowsim.medium.generate_multiple") as gen_mock:
        for result in trace_multiple(potential, 5, work_dir=str(tmpdir), potgen_options={}, ray_count=1000,
                                     initial_condition="planar",
                                     observers=[Caustics(True, file_name="caustic_file1.dat")]):
            pass
    gen_mock.assert_called_once_with(potential, 5, {"order": 2}, mock.ANY)
