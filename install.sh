#!/bin/bash
#
# This is a convenience script to facilitate easy installation in the most common circumstances.
#

# remember source dir as an absolute path
SOURCE_DIR=$(cd `dirname ${BASH_SOURCE}` && pwd)

# if we are in a virtual env, install to it
if [ -z "${VIRTUAL_ENV}" ]; then
  PREFIX=~/.local/
  PIP_ARGS="--user"
  if [ -z "${BUILD_DIRECTORY}" ]; then
    BUILD_DIRECTORY=/tmp/build-bsim
  fi
else
  PREFIX=${VIRTUAL_ENV}
  PIP_ARGS=""
  if [ -z "${BUILD_DIRECTORY}" ]; then
	BUILD_DIRECTORY=${VIRTUAL_ENV}/tmp/build-bsim
  fi
fi

# ensure build directory exists
if [ ! -d "$BUILD_DIRECTORY" ]; then
  mkdir -p ${BUILD_DIRECTORY}
fi

# if we set install prefix externally, we override 
# the default install dir
if [ -z ${INSTALL_PREFIX+x} ]; then
  PREFIX=${PREFIX}
else
  PREFIX=${INSTALL_PREFIX}
fi

export CXXFLAGS="-pipe -Wall -Wextra -pedantic -Wfloat-equal -mtune=native"
export CFLAGS=${CXXFLAGS}

cd ${BUILD_DIRECTORY}
cmake -g "Unix Makefiles" ${SOURCE_DIR}/       \
        -DCMAKE_BUILD_TYPE=Release             \
        -DPIP_ARGS="$PIP_ARGS"                 \
        -DCMAKE_INSTALL_PREFIX=${PREFIX}/ "$@"
make -j$(nproc) --no-print-directory
make install
