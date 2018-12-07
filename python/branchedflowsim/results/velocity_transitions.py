# -*- coding: utf-8 -*-
import numpy as np
from branchedflowsim.io import ResultFile, DataSpec


class VelocityTransitions(ResultFile):
    _FILE_HEADER_ = 'velt002\n'
    _FILE_NAME_ = 'velocity_transitions.dat'
    _SPEC_ = (DataSpec("num_bins", int),
              DataSpec("dimensions", int),
              DataSpec("time_interval", float),
              DataSpec("velocities", float, "num_bins"),
              DataSpec("counts", "grid", reduction="add"))

    def __init__(self, source):
        """ loads the velocity histogram data from the path base_path.
            If full path is True, uses base_path as a complete file path,
            otherwise the default file velocity_histograms.dat is opened.
        """
        super(VelocityTransitions, self).__init__(source)
