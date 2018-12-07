# -*- coding: utf-8 -*-
from branchedflowsim.io import ResultFile, DataSpec


class VelocityHistograms(ResultFile):
    _FILE_HEADER_ = 'velh001\n'
    _FILE_NAME_ = 'velocity_histograms.dat'
    _SPEC_ = (DataSpec("num_hists", int),
              DataSpec("num_bins", int),
              DataSpec("dimensions", int),
              DataSpec("times", float, "num_hists"),
              DataSpec("velocities", float, "num_bins"),
              DataSpec("counts", "grid", "num_hists", reduction="add")
              )

    def __init__(self, source):
        """ loads the velocity histogram data from the path base_path.
            If full path is True, uses base_path as a complete file path,
            otherwise the default file velocity_histograms.dat is opened.
        """
        super(VelocityHistograms, self).__init__(source)
