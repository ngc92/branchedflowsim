"""
Functions that are useful for quick and dirty testing of stuff.
"""
import branchedflowsim as bsim


def single_density(medium, num_rays, resolution=None, center=False, incoming="planar", end_time=1.0):
    """
    Traces rays on a single realization of potential to calculate a density field.
    
    :param branchedflowsim.MediumSpec medium: Description of the medium on which to trace.
    :param int num_rays: Number of rays for tracing.
    :param int resolution: Resolution of the histogram. If `None` use resolution of the medium.
    :param bool center: Whether to re-center the rays before density tracing.
    :param float end_time: Time to stop the tracing.
    :param str|branchedflowsim.incoming.Incoming incoming: Incoming wave form.
    :return np.ndarray: The density field as an array.
    """
    if resolution is None:
        resolution = medium.shape

    for result in bsim.trace_multiple(medium, repeat=1, ray_count=num_rays, initial_condition=incoming,
                                      end_time=end_time, periodic=True,
                                      observers=[bsim.observers.Density(center=center, size=resolution)]):
        return result.density.density
