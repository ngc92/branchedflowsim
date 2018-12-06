"""
In this example we will create a random potential and then perform ray tracing on
it to get the ray density distribution.
Then we will look at a more convenient way to handle multiple ray tracings on the
same medium, with the example of calculating the mean time to first caustic.
"""

import branchedflowsim as bsim
import matplotlib.pyplot as plt
import os

# start by defining the medium and creating a realization
corr = bsim.correlation.IsotropicGaussian(scale=0.1)
spec = bsim.ScalarPotentialSpec(256, dimension=2, correlation=corr, strength=0.1)
spec.create("medium.dat")

# define the ray initial condition for the tracing. In this case we
# use a planar initial condition with default parameters.
initial_condition = bsim.incoming.Planar()

# define the observers to be run. Observers are responsible for gathering data
# during the tracing. Here we use twice the resolution for the density than we use
# for the potential.
density_obs = bsim.observers.Density(size=512)

# Now do the tracing. We need to specify the potential either as a file name or a `Potential`
# object, the strength of the potential for tracing (if this differs from the strength with which
# the potential was saved, it will be rescaled), the number of rays to trace. We further
# pass in the initial condition and observers to used. Finally we also need a working path in which
# intermediate results are generated.
# we also disable periodic boundary conditions so rays end at the border of the density plot.
result = bsim.trace("medium.dat", strength=0.1, ray_count=20000, initial_condition=initial_condition,
                    path="trace-example", periodic=False, observers=[density_obs])

# `result` now contains all the data gathered by the observers. In most cases this can be simply accessed
# based on the type of the observer (e.g. the attribute `density` for a `Density` observer).
# This only breaks down if you specify multiple observers of the same type, in which case the more general
# `get_result` method needs to be used.
density = result.density

# density is a `ResultFile` object, that contains the actual density and a bit of metadata.
# we can access the density as a numpy array with the `density` attribute.
plt.imshow(density.density, origin="lower")
plt.show()

# clean up the generated files
os.remove("medium.dat")
for f in os.listdir("trace-example"):
    os.remove(os.path.join("trace-example", f))
os.rmdir("trace-example")

# now we want to calculate the mean time to first caustic. For this statistics we want to average over
# many different realizations of the same medium.

# To improve the amount of data we can gather from a single potential we perform tracing with a random
# planar initial condition, which means that the rays will emerge from random positions and in random
# directions but will be treated as if they came from a planar initial condition. For the calculation
# of the time to first caustic this is equivalent.
initial_condition = bsim.incoming.RandomPlanar()
caustic_obs = bsim.observers.Caustics(break_on_first=True)

total_caustics = 0
total_caustic_time = 0
# `trace_multiple` generates multiple realizations of the specified medium and performs
# the tracing on them. We can then iterate over the results.
for result in bsim.trace_multiple(spec, repeat=5, ray_count=10000,
                                  initial_condition=initial_condition, observers=[caustic_obs]):
    # we get the caustics result and do accumulation
    caustics = result.caustics
    total_caustics += len(caustics.times)
    total_caustic_time += sum(caustics.times)

print("Mean time to first caustic: %f" % (total_caustic_time / total_caustics))

# trace_multiple does all its work in a (configurable) temporary directory so there is no need for
# any manual cleanup here.
