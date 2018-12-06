# -*- coding: utf-8 -*-
"""
Created on Thu May  7 10:18:20 2015

@author: eriks

This file contains a class for managing ray trajectories
"""

from itertools import izip as izip

import numpy as np

from branchedflowsim.io import ResultFile, DataSpec


# this function creates a type descriptor for an entry inside a trajectory file
def trajectory_sample_type( dim ):
    """ this function creates a numpy type descriptor for trajectory samples of dimension
        dim. This makes it possible to use numpy.fromfile with this type to 
        read trajectories written by the trajectory observer.
    """
    entries = [("trajectory", np.uint64),
               ("position", np.double, (dim,)),
               ("velocity", np.double, (dim,)),
               ("time", np.double)]
    return np.dtype(entries)


class Trajectories(ResultFile):
    _FILE_HEADER_ = 'traj001\n'
    _FILE_NAME_ = 'trajectory.dat'
    _SPEC_ = (DataSpec("dimension", int),
              DataSpec("max_index", int, reduction="fail"),
              DataSpec("num_samples", int, is_attr=False),
              DataSpec("trajectories", lambda d: trajectory_sample_type(d["dimension"]), "num_samples",
                       reduction="concat")
              )

    def __init__(self, source):
        """ loads the trajectory data from the path base_path. If full path is True, uses base_path as
            a complete file path, otherwise the default file trajectory.dat is opened
        """
        super(Trajectories, self).__init__(source)

    def _from_dict(self, data):
        self.relative = False

        # utility references
        self.positions = self.trajectories['position']
        self.velocities = self.trajectories['velocity']
        self.times = self.trajectories['time']

    def _to_file(self, data):
        data["num_samples"] = len(self.trajectories)
    
    def find_initial_points(self):
        """ This function scans the trajectory points and saves the array index for the first point in the trajectory
        """
        
        self.starts = np.zeros(int(self.max_index)+1, dtype=int )
        traj = self.trajectories['trajectory'][0]
        for t, i in izip(self.trajectories['trajectory'], range(0, len(self.trajectories['trajectory']))):
            # subtract the starting position from each trajectory               
            if traj != t:
                # found new one
                traj = t
                self.starts[t] = i
        
        return self
        
    # make all positions relative
    def make_relative_positions( self ):
        """ substracts the initial ray position. Now th position variable contains the distance to the
            ray origin, not the coordinate origin
        """
        assert not self.relative     # prevent double subtraction

        # copy all origins, so we can overwrite in original array
        origins = np.zeros((int(self.max_index)+1, self.dimension))
        for s,i in izip(self.starts, range(0, int(self.max_index)+1)):
            origins[i] = self.positions[s].copy() # i don't think we need copy here, but better save than sorry

        for t, p in izip(self.trajectories['trajectory'], self.trajectories['position']):
            p -= origins[t]
        
        self.relative = True
        return self
    
    def make_trajectory_list( self ):
        """ converts the big trajectory points array in a list of arrays, where each array
            represents a single trajectory
        """
        
        if not hasattr(self, 'starts'):
            self.find_initial_points( )
        
        maxt = np.max(self.trajectories['trajectory'])
        trajlist = []
        
        for t in range(maxt+np.uint64(1)):
            trajlist.append( self.trajectories[ self.trajectories['trajectory'] == t ] )
        
        return trajlist
