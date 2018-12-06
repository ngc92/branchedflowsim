#!/usr/bin/env python

import os
import sys
import branchedflowsim as bsim

import matplotlib.pyplot as plt
import numpy as np
import argparse

parser = argparse.ArgumentParser(description="View volumetric branchedflowsim data in 3D")
parser.add_argument("density_file", type=str, help="Path to the density file to load.")
parser.add_argument("image_file", type=str, help="Save path for the generated image.")
args = parser.parse_args()


data = bsim.results.Density(args.density_file).density

# compensate for huge outliers which would screw up the color map
percent99 = np.percentile(data, 99)
data[data>percent99]=percent99

def savefig(data, target):
    # get image size for later use
    w = data.shape[0]
    h = data.shape[1]

    # create a figure with specified dpi. use figuresize so that 1pt = 1px
    dpi = 64
    fig = plt.figure(figsize=(w/dpi, h/dpi), dpi=dpi)
    fig.figimage(data, cmap="gray")

    # save to file with same dpi
    fig.savefig(target, dpi=dpi)


# show a cut of 3D data:
if len(data.shape) == 3:
    d = data.shape[2]
    fnm, ext = os.path.splitext(args.image_file)
    for (i, cut) in enumerate(range(d/20, d, d/10)):
	    savefig(data[cut, :, :], fnm+"%d"%i+ext)
else:
    savefig(data, args.image_file)


