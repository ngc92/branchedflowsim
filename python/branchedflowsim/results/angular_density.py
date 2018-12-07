
import numpy as np
from branchedflowsim.io import ResultFile, DataSpec


class AngularDensity(ResultFile):
    _FILE_HEADER_ = 'rade001\n'
    _FILE_NAME_ = 'angular_density.dat'
    _SPEC_ = (DataSpec("radii_count", int),
              DataSpec("resolution", int),
              DataSpec("radii", float, "radii_count"),
              DataSpec("counts", "grid", "radii_count", reduction="add"),
              )

    def __init__(self, source):
        super(AngularDensity, self).__init__(source)
