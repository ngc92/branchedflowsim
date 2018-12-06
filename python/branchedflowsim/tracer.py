from __future__ import absolute_import

import logging
import shutil
import subprocess
import os
import tempfile
from collections import defaultdict

from branchedflowsim import config
from branchedflowsim.observers.observer import Observer
from branchedflowsim.incoming.incoming import Incoming
from branchedflowsim import medium
from branchedflowsim.potential import Potential

_logger = logging.getLogger(__name__)


class Integrator:
    """
    Defines constants for the different integrators that can be used for ray tracing.
    Currently possible:
    * `CONST_EULER`: An euler integrator (x' = x + dt*(dx/dt)) with constant time steps.
    * `ADAPTIVE_RUNGE_KUTTA`: A 4'th order runge kutta integrator (cash-karp54) with \
        5th order error tracking and adaptive step size.
    """
    CONST_EULER = "euler"
    ADAPTIVE_RUNGE_KUTTA = "adaptive"


def make_trace_command(potential, strength, count, initial_condition, path, periodic=True, end_time=2.0, dynamics=None,
                       threads=None, observers=None, integrator=None, args=None):
    """ 
    This function builds the command line for a tracing call.
    
    :param str potential: The name of the potential file.
    :param strength: The strength of the potential.
    :param int count: Number of rays to trace.
    :param str|Incoming initial_condition: Initial conditions for rays. Either an `Incoming` object, or a single string.
    :param str path: Path where to put the result files.
    :param bool periodic: Whether to enable periodic boundary conditions.
    :param float end_time: Ending time for tracing.
    :param str dynamics: Tracing dynamics specified as for the command line (space separated string)
    :param int threads: Number of threads. If not set, use all available cores.
    :param list[Observer] observers: A list of observers.
    :param str integrator: The integrator to be used for ray tracing. See `tracer.Integrator`.
    :param list[str] args: Additional arguments to be passed.
    :return list[str]: The corresponding command line invocation as a list of arg values.
    """
    trcmd = [config.trace_exe, potential,
             '-n', str(count),
             '-s', str(strength),
             '-r', str(path),
             '--end-time', str(end_time)]

    trcmd += ['--incoming']
    # source type
    if isinstance(initial_condition, Incoming):
        trcmd += initial_condition.make_command()
    else:
        trcmd += initial_condition.split(" ")

    if periodic:
        trcmd += ['--periodic']

    if threads:
        trcmd += ['--threads', str(threads)]

    if not (dynamics is None):
        trcmd += ["--dynamics"]
        trcmd += dynamics.split(" ")

    if observers:
        trcmd += ['--observers']
        for observer in observers:  # type: Observer
            trcmd += observer.make_command()

    if integrator is not None:
        trcmd += ["--integrator", str(integrator)]

    if args:
        trcmd += args

    return trcmd


def trace(potential_file, strength, ray_count, initial_condition, path, periodic=True, end_time=2.0, dynamics=None, threads=None,
          observers=None, integrator=None, args=None):
    """ calls the external trace program.
    
    :param str|Potential potential_file: Path to the file that contains the potential. If a `Potential` object \
           is passed its `file_name` attribute is extracted.
    :param float strength: Strength of the potential.
    :param int ray_count: Number of rays to trace.
    :param Incoming initial_condition: Initial conditions for rays.
    :param str path: Path where to put the result files.
    :param bool periodic: Whether to enable periodic boundary conditions.
    :param float end_time: Ending time for tracing.
    :param str dynamics: Tracing dynamics specified as for the command line (space separated string)
    :param int threads: Number of threads. If not set, use all available cores.
    :param list[Observer] observers: A list of observers.
    :param str integrator: The integrator to be used for ray tracing. See `tracer.Integrator`.
    :param list[str] args: Additional arguments to be passed.
    :return: A trace result object for lazily loading the results produced by the observers.
    :rtype:  TraceResult
    :raises: CalledProcessError If the trace program encounters an error.
    """
    if isinstance(potential_file, Potential):
        potential_file = potential_file.file_name
    command = make_trace_command(potential_file, strength, ray_count, initial_condition, path, periodic,
                                 end_time, dynamics, threads, observers, integrator=integrator, args=args)

    # trace trajectories
    _logger.info("Start tracing %d rays on potential %s", ray_count, potential_file)
    _logger.debug("Calling tracer: %s", " ".join(command))

    try:
        log = subprocess.check_output(command)
        _logger.debug("tracer output:\n%s", log)
    except subprocess.CalledProcessError as err:
        _logger.error(err.output)
        raise

    return TraceResult(path, observers)


def trace_multiple(medium_spec, repeat, ray_count, work_dir=None, potgen_options=None, **kwargs):
    """
    perform multiple tracings with the same settings on different potential realizations.
    This will generate multiple versions of the potential (`potential.copy` followed by `potential.create`)
    and trace on those.

    All files are created in a temporary directory (under `work_dir`) that is deleted once the function finishes.
    If you need to manually look at the potential realizations or want to keep the result files, do your tracing
    using the `trace` function.

    :param MediumSpec medium_spec: Specifies the medium on which to trace.
    :param int repeat: The number of potential realizations to use.
    :param work_dir: A directory in which all files will be created.
                     If `None` uses `branchedflowsim.config.DEFAULT_WORKDIR`.
    :param dict potgen_options: Options to be passed tp the `create` function of the potential. \
                                Note that specifying a `seed` other than `None` defeats the purpose of multiple \
                                tracings.
    :param int ray_count: Number of rays to trace per medium realization.
    :param kwargs: All additional keyword args will be passed through to the `trace` call.

    :return: A generator for the tracing results.
    """
    if potgen_options is None:
        potgen_options = {}

    # if any of the observers require monodromy, we need the potential up to second order.
    # Otherwise first order is sufficient.
    if "order" not in potgen_options:
        if any(observer.need_monodromy for observer in kwargs["observers"]):
            potgen_options["order"] = 2
        else:
            potgen_options["order"] = 1

    work_dir = tempfile.mkdtemp(dir=config.get_workdir(work_dir))
    try:
        _logger.info("Performing %d tracings in temporary directory %s", repeat, work_dir)
        for potential in medium.generate_multiple(medium_spec, repeat, potgen_options, work_dir):
            yield trace(potential_file=potential, strength=medium_spec.strength,
                        ray_count=ray_count, path=work_dir, **kwargs)
    finally:
        shutil.rmtree(work_dir)
        _logger.info("Cleaned up temporary directory %s", work_dir)


def reduce_trace_multiple(*args, **kwargs):
    """
    Invokes `trace_multiple` with the supplied arguments and performs a reduction operation on all the returned results.

    :return: The results reduced over all realisations, as a dictionary.
    :rtype: Dict[str, ResultFile]
    """
    results = defaultdict(lambda: None)
    result_iterator = trace_multiple(*args, **kwargs)
    for result in result_iterator:
        for key in result.keys():
            results[key] = result.get_result(key).reduce(results[key])
    return results


def _observer_result_mapping():
    """
    This function returns a map from observer names to result loaders.
    
    :rtype: dict
    """
    from branchedflowsim.results import VelocityTransitions
    from branchedflowsim.results import VelocityHistograms
    from branchedflowsim.results import Trajectories
    from branchedflowsim.results import Caustics
    from branchedflowsim.results import AngleHistograms
    from branchedflowsim.results import Density
    from branchedflowsim.results import AngularDensity
    mapping = {
        "caustics": Caustics,
        "density": Density,
        "trajectory": Trajectories,
        "angle_histogram": AngleHistograms,
        "velocity_histogram": VelocityHistograms,
        "velocity_transitions": VelocityTransitions,
        "radial_density": AngularDensity
    }
    return mapping


def _make_loader(result_type, base_path, file_name):
    """
    Takes a target type, base path and file name and returns a function that loads from that file.
    Used to implement lazy loading of results.
    
    :param result_type: Type of the result. Should be one of the `ResultFile` classes.
    :param base_path: Directory for the result files.
    :param file_name: Filename of the result file. If None the default file name will be used.
    :return: A function that loads the data.
    """
    if file_name is None:
        path = base_path
    else:
        path = os.path.join(base_path, file_name)

    def loader():
        _logger.debug("Loading %s from file %s", result_type.__name__, path)
        return result_type(path)

    return loader


class TraceResult(object):
    """
    This class is responsible for lazily loading the data that was collected
    by the observers during a tracing run. No files are read during construction
    of the `TraceResult` object. On the first access of one of the result properties
    the corresponding file is read and cached. Any subsequent access only returns
    the cached object.

    Results for observers for which you manually set the `file_name` can be retrieved
    by calling `get_result(FILE_NAME)`, where `FILE_NAME` is the exact file name you specified
    (i.e. excluding the result directory, and including its extension)

    Note that this requires that the result files still be present when loading
    is required. If you want to remove the result folder but get the data beforehand
    you can use `load_files` which caches all results.

    TODO track more data about the tracing configuration (incoming, raycount etc).
    """

    def __init__(self, base_path, observers):
        self.basepath = base_path  # path in which all result files are saved

        self._loaded_cache = {}
        self._loaders = {}

        mapping = _observer_result_mapping()

        for obs in observers:  # type: Observer
            loader = _make_loader(mapping[obs.name], self.basepath, getattr(obs, "file_name", None))

            # if we got a file_name, register the loader under that name, else use the default (=observer name).
            if getattr(obs, "file_name", None):
                self._loaders[obs.file_name] = loader
            else:
                self._loaders[obs.name] = loader

    def _lazy_load(self, name):
        """
        Loads results corresponding to `name` from file if they have not already been loaded, otherwise returns a
        cached result.
        :param name: Name of the observer whose results to load, or name of the save file to load.
        :return: The results produced by the specified observer.
        :rtype: branchedflowsim.io.ResultFile
        """
        if name in self._loaded_cache:
            return self._loaded_cache[name]

        if name in self._loaders:
            self._loaded_cache[name] = self._loaders[name]()
            return self._loaded_cache[name]

        return None

    def keys(self):
        return self._loaders.viewkeys()

    def get_result(self, name):
        return self._lazy_load(name)

    @property
    def density(self):
        """:rtype: branchedflowsim.results.Density"""
        return self._lazy_load("density")

    @property
    def radial_density(self):
        """:rtype: branchedflowsim.results.Radial_Density"""
        return self._lazy_load("radial_density")

    @property
    def caustics(self):
        """:rtype: branchedflowsim.results.Caustics"""
        return self._lazy_load("caustics")

    @property
    def trajectories(self):
        """:rtype: branchedflowsim.results.Trajectories"""
        return self._lazy_load("trajectory")

    @property
    def angle_histogram(self):
        """:rtype: branchedflowsim.results.AngleHistograms"""
        return self._lazy_load("angle_histogram")

    @property
    def velocity_histogram(self):
        """:rtype: branchedflowsim.results.VelocityHistograms"""
        return self._lazy_load("velocity_histogram")

    @property
    def velocity_transitions(self):
        """:rtype: branchedflowsim.results.VelocityTransitions"""
        return self._lazy_load("velocity_transitions")

    def load_files(self):
        for name in self._loaders:
            self._lazy_load(name)

    def __repr__(self):
        return "<TraceResult %s>" % " ".join(self._loaders.viewkeys())
