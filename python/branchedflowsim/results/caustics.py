# -*- coding: utf-8 -*-
"""
Created on Wed May  6 14:29:05 2015

@author: eriks

This file contains classes and functions to work with the result provided 
by the caustic observer. Several standard / often needed operations on
caustic data are provided.
"""

import numpy as np

from branchedflowsim.io import ResultFile, DataSpec


# this helper function creates a type descriptor for an entry inside a caustic file
def caustic_type(dim):
    """ this function creates a numpy type descriptor for caustics of dimension
        dim. This makes it possible to use numpy.fromfile with this type to
        read caustics written by the caustic observer.
    """
    entries = [("trajectory", np.uint64),
               ("position", np.double, (dim,)),
               ("velocity", np.double, (dim,)),
               ("origin", np.double, (dim,)),
               ("original_velocity", np.double, (dim,)),
               ("time", np.double),
               ("index", np.uint8)]
    return np.dtype(entries)


class Caustics(ResultFile):
    _FILE_NAME_ = 'caustics.dat'
    _FILE_HEADER_ = 'caus001\n'
    _SPEC_ = (DataSpec("raycount", int, reduction="add"),
              DataSpec("dimension", int),
              DataSpec("caustic_count", int, is_attr=False),
              DataSpec("caustics", lambda d: caustic_type(d["dimension"]), "caustic_count", reduction="concat")
              )

    def __init__(self, source):
        """ loads the caustic data from the path cfile. If full path is True, uses cfile as
            a complete file path, otherwise the default file caustics.dat is opened
        """
        super(Caustics, self).__init__(source)

    def _to_file(self, data):
        data["caustic_count"] = len(self.caustics)

    # discard all caustics with index != idx. First caustic index is 1
    def select_index( self, index, apply = True ):
        """ set apply to False if you want to keep all caustics in the current instance and
            just want the filtered positions returned
        """
        idx = self.caustics['index'] == index
        filtered = self.caustics.compress(idx)
        if apply:
            self.caustics = filtered
            # reset cached values
            self._mean_time = None
            self.positions = self.caustics['position']
            self.times = self.caustics['time']
        
        return filtered
    
    # make all caustic positions relative
    def make_relative_positions( self ):
        """ substracts the initial ray position. Now th position variable contains the distance to the
            ray origin, not the coordinate origin
        """
        assert( not self.relative )     # prevent double subtraction
        self.positions -= self.caustics['origin']
        self.relative = True
        return self

    def _from_dict(self, data):
        self.relative = False

        # set helpful links
        self.positions = self.caustics['position']
        self.times = self.caustics['time']
    
    def mean_time(self):
        """ returns the mean time of all caustics in this collection. The result is cached, so multiple calls
            to this function will not harm performance
        """
        if self._mean_time is None:
            self._mean_time = np.mean(self.times)
        return self._mean_time
    
    def get_time_for_origin(self, use_vel = False):
        """ returns an array that combines the ray origin information with the 
            time to first caustic;
            if use_vel == True, uses initial velocity instead of position as origin
            information
        """
        result = np.empty((len(self.times), 1+len(self.positions[0])))
        for i in range(len(self.positions[0])):
            if use_vel:
                result[:, i] = self.caustics['original_velocity'][:, i]
            else:
                result[:, i] = self.caustics['origin'][:, i]
        result[:, -1] = self.times
        return result
    
    def get_position_on_initial_manifold(self):
        """ This function creates an array that contains the two dimensional indices
            of the rays on the initial manifold. 
            This function caches the calculated array, and can thus be called multiple times
            without harming performance. 
            assumes that the initial condition was a planar wave
            
            note: This can be used to attribute information
            to initial states in a N-1 dimensional array. 
            see function attribute_to_init for an example of for what this is 
            useful
        """
        
        if hasattr(self, '_pos_on_imanifold'):
            return self._pos_on_imanifold
        
        # total width of configuration (i.e. taking into account safety boundary)
        db = np.max(self.caustics['origin'][:, 1]) - np.min(self.caustics['origin'][:, 1])
        
        # we assume that the initial condition was a planar wave.
        if len(self.positions[0]) == 3:
            # get y and z coordinates of the ray origins (x is const)
            pos_on_imanifold = np.copy(self.caustics['origin'][:, (1,2)])
            # this is the separation of neighbouring rays.
            isep = db/(np.sqrt(self.raycount)-1)
        elif len(self.positions[0]) == 2:
            # get y coordinate of the ray origins (x is const)
            pos_on_imanifold = np.copy(self.caustics['origin'][:, 1])
            # this is the separation of neighbouring rays.
            isep = db/(self.raycount-1.0)
        else:
            assert('unsupported dimension')
        
        
        # subtract the border and rescale data according to separation
        pos_on_imanifold -= np.min(pos_on_imanifold, axis=0) 
        pos_on_imanifold /= isep
        
        # now convert to integers
        pos_on_imanifold = np.round(pos_on_imanifold).astype(int)
        
        self._pos_on_imanifold = pos_on_imanifold
        
        return pos_on_imanifold
    
    def attribute_to_init(self, attribute):
        """ creates a two dimensional array that contains the data referenced by attribute, but is
            indexable by the ray origin instead of the caustic number.
            
            if attribute is a string, it has to be one of trajectory, position, velocity, origin, original_velocity, time, index,
            i.e. those used by entries in the caustic_type function (note that not all of those make sense).
            
            otherwise, attribute has to be an array of the same length as caustics.
            
            This function will not work correctly if there are rays which 
            contain more than a single caustic.
            TODO allow multiple attributes
        """

        
        # get  initial manifold positions
        pos_on_imanifold = self.get_position_on_initial_manifold()
        
        # try if the attribute is an attribute index, if yes get the corresponding array
        try:
            attribute = self.caustics[attribute]
        except:
            pass
        
        # allocate the array
        info_len = np.size(attribute[0])
        if len(self.positions[0]) == 3:
            index_array = np.zeros((np.max(pos_on_imanifold, axis=0)[0]+1, np.max(pos_on_imanifold, axis=0)[1]+1, info_len))
            for i in range(len(self.caustics)):
                index_array[pos_on_imanifold[i][0], pos_on_imanifold[i][1]][:] = attribute[i]
        elif len(self.positions[0]) == 2:
            index_array = np.zeros((np.max(pos_on_imanifold)+1, info_len))
            for i in range(len(self.caustics)):
                index_array[pos_on_imanifold[i]][:] = attribute[i]
        else:
            assert('unsupported dimension')
        
        
        return index_array

