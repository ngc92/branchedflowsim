# -*- coding: utf-8 -*-
import numpy as np
from branchedflowsim.io import ResultFile, DataSpec


class Density(ResultFile):
    _FILE_HEADER_ = 'dens001\n'
    _FILE_NAME_ = 'density.dat'
    _SPEC_ = (DataSpec("dimensions", int),
              DataSpec("support", float, "dimensions"),
              DataSpec("density", "grid", reduction="add"))

    def __init__(self, source):
        super(Density, self).__init__(source)
