# Branchedflowsim{#mainpage}

A program for simulating branched flows through ray-tracing, as well as generating the underlying potential of a random medium.

## Installation Instructions
Make sure you clone this repository including its submodules (e.g. using 
`git clone --recurse-submodules`). Then, if your do not want to change the default build
configuration you can execute `./install.sh` in the main project directory. 
You can specify the build and installation directories by setting the `BUILD_DIRECTORY` 
and `INSTALL_PREFIX` environment variables to the desired paths.
By default the installation will be done locally in your home directory `~/.local/` so there
is no need for superuser privileges. If the install script is run while in an active python 
virtualenv, installation will target the folder of that environment.

### Prerequisites
A recent C++ compiler (e.g. g++ >= 5.0), boost (1.58), cmake (>= 3.5) fftw3, as 
well as pip for the installation of the python library and doxygen and latex to
build the documentation.

On a recent ubuntu system, everything should be ready after
```
apt-get install -y cmake g++ libboost-all-dev fftw3 libfftw3-dev python-pip doxygen texlive
```


### Manual Building
Branchedflowsim uses cmake as its built system for both the c++ library and programs as well as the
python library. Please take a look at `./install.sh` to see how the installation can be configured.
Note that `make install` will also include the installation of the python library via `pip`, if
possible.

## Documentation

### Developer documentation
After installation, the documentation for the C++ code is available at 
`${INSTALL_PREFIX}/share/doc/branchedflowsim`.

The documentation for the most recent version can also be found online 
[here](https://erik.schultheis.pages.gwdg.de/branchedflowsim/)

### Python API Docs
The python API documentation will be installed to
`${INSTALL_PREFIX}/share/doc/branchedflowsim/python-api`.
The online version can be found
[here](https://erik.schultheis.pages.gwdg.de/branchedflowsim/python_api).

### Usage Examples
You can get information about the program by using the help command, 
e.g. `potgen --help` will show you all available options for the `potgen` 
command.
For example
```bash
potgen -d 2 -s 256 -l 0.1 -o pot.dat
tracer pot.dat -n 10000 -s 0.1
density_image result/density.dat density.png
```
will create a 256x256 normally distributed random potential with Gaussian correlation 
of correlation length 0.1 and save it in `pot.dat`. Then we will perform ray tracing of
10000 rays, on a version of the potential scaled down to have standard deviation of 0.1.
A `results` folder will be created that contains a `config.txt` that tells you the 
parameters of the tracing. It also contains the file `density.dat` which contains the 
calculated ray density. (Since we did not specify any "observer" for the tracing the default
of calculating the density was chosen).
To visualize this density we create a png file using the `density_image` script.

If you want to quantitatively work with the tracing results it is highly recommended to use
the `branchedflowsim` python package. This facilitates both loading of the result files as
well as calling the `potgen` and `tracer` programs with the correct arguments to produce the 
data. Usage examples can be found in the `python/examples` subdirectory.




### Programs
Installing branchedflowsim installs the following programs that can be 
used from the command line:
  * *potgen* Generate random potentials
  * *tracer* Ray tracing on random potentials
  * *density_image* Read in the ray density and plot as an image.
  * *viewvolume* view volumetric data
