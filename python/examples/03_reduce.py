"""
In this example we look at the process of calculating the mean time to the first caustic as
a reduction operation.
"""

import branchedflowsim as bsim

# medium, initial condition and observer
corr = bsim.correlation.IsotropicGaussian(scale=0.1)
spec = bsim.ScalarPotentialSpec(256, dimension=2, correlation=corr, strength=0.1)
initial_condition = bsim.incoming.RandomPlanar()
caustic_obs = bsim.observers.Caustics(break_on_first=True)

# this call performs the multiple tracings as in example `02_trace.py` but additionally combines
# the results of observer using their reduce operation. This makes for very convenient code, but
# two things are to be considered here:
#  1) not every observer result does have a canonical way of defining a reduction operation
#  2) in the case of caustics, the reduction means concatenating all found caustics into a really
#     long list. In situations where many repeats are needed this can significantly increase the
#     amount of required memory.
result = bsim.reduce_trace_multiple(spec, repeat=5, ray_count=10000,
                                    initial_condition=initial_condition, observers=[caustic_obs])

# the `reduce_trace_multiply` function returns a simple dict, as all data loading has already been
# performed during the reduction operation so no lazy loading code is needed anymore.
caustics = result["caustics"]

print("Mean time to first caustic: %f " % (sum(caustics.times) / len(caustics.times)))