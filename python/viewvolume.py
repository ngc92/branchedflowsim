#!/usr/bin/env python

import argparse
import numpy as np
import scipy.ndimage
from mayavi import mlab
from tvtk.util.ctf import ColorTransferFunction
import branchedflowsim as bsim

parser = argparse.ArgumentParser(description="View volumetric branchedflowsim data in 3D")
parser.add_argument("file", type=str, help="File to load.")
parser.add_argument("--cutoff", type=float, help="Cutoff (percentile) for maximum value in data", default=99)
parser.add_argument("--vmin", type=float, help="Lower cutoff for transparency", default=0.0)
parser.add_argument("--vmax", type=float, help="Upper cutoff for transparency", default=1.0)
parser.add_argument("--show-cut", help="Show a planar cut", action='store_true')
parser.add_argument("--show-iso", help="Show an iso-surface", type=float, default=None)
parser.add_argument("--fix-r2", help="multiplies each data point by r^2", action='store_true')
args = parser.parse_args()


def load_data(filename, subaccess=None):
    if bsim.is_potential_file(filename):
        pot = bsim.Potential.from_file(open(filename, "rb"))
        if subaccess is None:
            subaccess = "potential"
        return pot.get_field(subaccess).field.astype(np.float32)
    elif bsim.is_velocity_histogram(filename):
        vel_hist = bsim.results.VelocityHistograms(filename)
        return np.array(vel_hist.counts).astype(np.float32)
    elif bsim.is_velocity_transitions(filename):
        vel_hist = bsim.results.VelocityTransitions(filename)
        return np.squeeze(np.array(vel_hist.counts).astype(np.float32))
    else:
        return bsim.read_grid(open(filename, "rb"))

print("loading data ...")
data = load_data(args.file)
print("done")

print("preprocessing...")
if args.fix_r2:
    for x in range(data.shape[0]):
        y = np.arange(data.shape[1])
        z = np.arange(data.shape[2])
        y, z = np.meshgrid(y, z)
        dx = (x - data.shape[0]/2.0) / data.shape[0]
        dy = (y - data.shape[1]/2.0) / data.shape[1]
        dz = (z - data.shape[2]/2.0) / data.shape[2]
        r2 = dx*dx + dy*dy + dz*dz
        data[x, y, z] *= r2
        

# compensate for huge outliers which would screw up the color map
cutoff = np.percentile(data, args.cutoff)
data[data>cutoff] = cutoff
mx = np.max(data)
mn = np.min(data)
data = ((data - mn) / (mx - mn)).astype(np.float32)

print("done")

volume_figure = mlab.figure(bgcolor=(1, 1, 1), fgcolor=(0,0,0))
vol = mlab.pipeline.volume(mlab.pipeline.scalar_field(data), vmin=args.vmin, vmax=args.vmax)

ctf = ColorTransferFunction()
ctf.add_rgb_point(0, 1, 1, 1)
ctf.add_rgb_point(1, 0, 0, 0.3)

vol._volume_property.set_color(ctf)
vol._ctf = ctf
vol.update_ctf = True
mlab.outline()

if args.show_cut:
    cut_figure=mlab.figure(bgcolor=(1, 1, 1),fgcolor=(0,0,0))
    mlab.pipeline.image_plane_widget(mlab.pipeline.scalar_field(data),
                                plane_orientation='x_axes',
                                slice_index=data.shape[0]/2,
                                figure=cut_figure
                            )
    mlab.sync_camera(volume_figure, cut_figure)
    mlab.outline()

if args.show_iso:
    # downsample to get a somewhat smoother surface
    downscaled = scipy.ndimage.zoom(data, 256.0 / data.shape[0])

    # do some blurring to get smoother contours
    scipy.ndimage.gaussian_filter(downscaled, 2.0, output = downscaled)
    iso_figure=mlab.figure(bgcolor=(1, 1, 1), fgcolor=(0,0,0))
    iso_surface = mlab.pipeline.iso_surface(mlab.pipeline.scalar_field(downscaled), 
                                            contours=[args.show_iso],
                                            figure=iso_figure)
    iso_surface.compute_normals = False # seems to look better this way
    mlab.outline(extent=(0,255, 0,255, 0,255))

mlab.show()
