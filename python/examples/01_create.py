"""
In this example a Gaussian random medium is generated using the `potgen` programme,
loaded into python and then show with matplotlib.
"""
import branchedflowsim as bsim
import matplotlib.pyplot as plt
import os

# create a medium specification that defines the physical parameters
# that will be used for the medium. For the scalar potential we need
# to define the size and dimension of the medium, the correlation
# function and the strength of the potential.
corr = bsim.correlation.IsotropicGaussian(scale=0.1)
spec = bsim.ScalarPotentialSpec(256, dimension=2, correlation=corr, strength=0.1)

# Create a potential that corresponds to the given specifications and
# save it in "medium.dat". Can optionally pass a `seed` parameter
# to make reproducible potentials.
# After generation it loads the new potential from the file.
# Note that this operation is done lazily, i.e. at this point
# only metadata is read from "medium.dat" and the whole file
# is only read in when the actual data is accessed.
potential = spec.create("medium.dat")

# since we have created a scalar potential, it contains only
# one filed that can be accessed using the `field` attribute.
# For a more complicated function we would need to use the
# `get_field` method with the corresponding field name.
scalar_potential = potential.field

# `scalar_potential` now contains all data about the potential.
# this includes the potential itself and its derivatives.
values = scalar_potential.field

# to get a derivative you need to pass in how often you want
# to derive with respect to the given variables.
dVdx = scalar_potential.get_partial_derivative(1, 0)

# show the result
plt.imshow(values, origin="lower")
plt.show()

# cleanup
os.remove("medium.dat")

# if you want to generate multiple potentials for the same MediumSpec
# and only need to do some computations on that potential and then
# can discard it again, you can use the `generate_multiple` function.
for potential in bsim.generate_multiple(spec, repeat=5):
    # we print the resulting file name here. you can see
    # that the file gets overwritten in each iteration.
    print(potential.file_name)
    print(potential.field.field[0, 0])

# we do not need to remove any leftover potentials here, as `generate_multiple`
# does automatic cleanup.
