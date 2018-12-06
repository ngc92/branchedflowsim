# - Try to find FFTW
# Once done this will define
#  FFTW_FOUND - System has FFTW
#  FFTW_INCLUDE_DIRS - The FFTW include directories
#  FFTW_LIBRARIES - The libraries needed to use FFTW

if( NOT FFTW_ROOT AND DEFINED ENV{FFTW_ROOT} )
	set( FFTW_ROOT $ENV{FFTW_ROOT} )
endif()

if( FFTW_ROOT )
	find_path(FFTW_INCLUDE_DIR fftw3.h PATHS ${FFTW_ROOT} PATH_SUFFIXES include NO_DEFAULT_PATH )
	find_library(FFTW_BASE_LIB NAMES fftw3 PATHS ${FFTW_ROOT} PATH_SUFFIXES lib lib64 NO_DEFAULT_PATH)
	find_library(FFTW_THREADS_LIB NAMES fftw3_threads PATHS ${FFTW_ROOT} PATH_SUFFIXES lib lib64 NO_DEFAULT_PATH)
else()
	find_path(FFTW_INCLUDE_DIR fftw3.h PATH_SUFFIXES include )

	find_library(FFTW_BASE_LIB NAMES fftw3 PATHS /usr/lib/x86_64-linux-gnu PATH_SUFFIXES lib lib64)
	find_library(FFTW_THREADS_LIB NAMES fftw3_threads PATHS /usr/lib/x86_64-linux-gnu  PATH_SUFFIXES lib lib64)
endif( FFTW_ROOT )

#set libraries: on unix systems, threads is separate
if( UNIX )
	set(FFTW_LIBRARIES ${FFTW_THREADS_LIB} ${FFTW_BASE_LIB})
else(UNIX)
	set(FFTW_LIBRARIES ${FFTW_BASE_LIB})
endif(UNIX)

set(FFTW_INCLUDE_DIRS ${FFTW_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set FFTW_FOUND to TRUE
# if all listed variables are TRUE
# FFTW_THREADS_LIB is only required on LINUX systems
if(UNIX)
	find_package_handle_standard_args(FFTW  DEFAULT_MSG FFTW_BASE_LIB FFTW_THREADS_LIB FFTW_INCLUDE_DIR)
else(UNIX)
	find_package_handle_standard_args(FFTW  DEFAULT_MSG FFTW_BASE_LIB FFTW_INCLUDE_DIR)
endif(UNIX)

# Make imported targets
add_library(fftw::fftw INTERFACE IMPORTED)
set_property(TARGET fftw::fftw APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${FFTW_INCLUDE_DIRS})
set_property(TARGET fftw::fftw APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${FFTW_LIBRARIES})

mark_as_advanced(FFTW_BASE_LIB FFTW_THREADS_LIB FFTW_INCLUDE_DIR)
