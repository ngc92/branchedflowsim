from __future__ import absolute_import
import random
import logging
import subprocess
from numbers import Integral

from branchedflowsim import config

_logger = logging.getLogger(__name__)


def make_potgen_command(file_name, dimension, size, correlation, strength, seed, order, fft_threads, profile, wisdom,
                        extra_args):
    """
    For a description of the arguments see `generate`.
    
    :return: A list of arguments to be used in a subprocess call.
    :rtype: list[str]
    """
    if not isinstance(size, Integral):
        if len(size) != dimension:
            raise ValueError("Submitted size %s invalid for not %s dimensional" % (size, dimension))
        size_arg = list(map(str, size))
    else:
        size_arg = [str(size)]

    command = [config.potgen_exe,
               '-d', str(dimension),
               '-s'] + size_arg + [
                  '--derivative-order', str(order),
                  '--threads', str(fft_threads),
                  '--output', str(file_name),
                  '--seed', str(seed),
                  '--strength', str(strength)
              ]

    if profile:
        command.append("--print-profile")

    if not wisdom:
        command.append("--no-wisdom")

    command += correlation.args
    command += list(map(str, extra_args))

    return command


def generate(file_name, dimension, size, correlation, strength=1.0, seed=None, order=2, fft_threads=None, profile=None,
             wisdom=None, extra_args=None):
    """
    Generate a new potential using the `potgen` program.
    
    :param str file_name: File in which the new potential is saved.
    :param int dimension: Dimension. Should be 2 or 3.
    :param int|Sequence[int] size: Size of the potential.
    :param Correlation correlation: Correlation function to be used.
    :param strength: Strength of the generated potential.
    :param int seed: A seed for the random number generation. Leave at `None` to use a random seed.
    :param int order: The highest order of derivatives to include. Defaults to 2 so we can do monodromy tracing \
        on the generated potential. If you don't need to calculate the monodromy matrix, this can be \
        lowered to 1 to save disk space and computation time.
    :param int fft_threads: Number of threads to use in the fft. \
        If not set the default value given in `config.DEFAULT_FFT_THREADS` is used.
    :param bool profile: Whether to print some profiling information. For debugging. \
        If no value is given uses `config.DEFAULT_ENABLE_PROFILE`.
    :param bool wisdom: Set to False to disable use of FFTWs wisdom to speed up FFT calculations.
    :param extra_args: Any additional command line arguments to be passed to `potgen`.
    :return: Nothing. The file is created on disk.
    """
    if seed is None:
        seed = random.getrandbits(32)

    if fft_threads is None:
        fft_threads = config.DEFAULT_FFT_THREADS

    if profile is None:
        profile = config.DEFAULT_ENABLE_PROFILE

    if wisdom is None:
        wisdom = config.DEFAULT_USE_FFTW_WISDOM

    if extra_args is None:
        extra_args = []

    command = make_potgen_command(file_name, dimension, size, correlation, strength, seed, order, fft_threads, profile,
                                  wisdom, extra_args)

    _logger.info("Generate %d dimensional potential %s", dimension, file_name)
    _logger.debug("Calling potgen: %s", " ".join(command))
    try:
        log = subprocess.check_output(command)
        _logger.debug("potgen output:\n%s", log)
    except subprocess.CalledProcessError as err:
        _logger.error(err.output)
        raise
