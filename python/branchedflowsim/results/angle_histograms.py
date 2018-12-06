# -*- coding: utf-8 -*-
"""
Created on Thu May  7 10:18:20 2015

@author: eriks

This file contains a class for managing ray trajectories
"""

import numpy as np
from branchedflowsim.io import ResultFile, DataSpec


class AngleHistograms(ResultFile):
    _FILE_HEADER_ = 'angh001\n'
    _FILE_NAME_ = 'angle_histograms.dat'
    _SPEC_ = (DataSpec("num_hists", int),
              DataSpec("num_bins", int),
              DataSpec("times", float, "num_hists"),
              DataSpec("angles", float, "num_bins"),
              DataSpec("sum_angles", float, "num_hists", reduction="add"),
              DataSpec("sum_angles_squared", float, "num_hists", reduction="add"),
              DataSpec("counts", int, ("num_hists", "num_bins"), reduction="add")
              )

    def __init__(self, source):
        super(AngleHistograms, self).__init__(source)
    
    def root_mean_square_angle(self):
        """ gets the root mean square of the angle in each histogram """
        msa = np.zeros(self.num_hists)
        for hid in range(self.num_hists):
            msa[hid] = self.sum_angles_squared[hid] / np.sum(self.counts[hid, :])
        return np.sqrt(msa)

