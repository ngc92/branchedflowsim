include(sphinx)

set(PYTHON_DEFAULT_FFT_THREADS 1 CACHE STRING "default number of threads python scripts will request for fft")
set(PYTHON_DEFAULT_WORK_DIR "/dev/shm" CACHE PATH "path to the default directory the python lib shall use for intermediate files. Defaults to /dev/shm for speed")

set(PYTHON_SOURCE branchedflowsim/__init__.py
    branchedflowsim/correlation.py
        branchedflowsim/medium.py
        branchedflowsim/potgen.py
        branchedflowsim/potential.py
        branchedflowsim/tracer.py
        branchedflowsim/cmdline_helpers.py
        branchedflowsim/io/__init__.py
        branchedflowsim/io/data_spec.py
        branchedflowsim/io/read_data.py
        branchedflowsim/io/result_file.py
        branchedflowsim/io/write_data.py
        branchedflowsim/observers/__init__.py
        branchedflowsim/observers/observer.py
        branchedflowsim/incoming/__init__.py
        branchedflowsim/incoming/incoming.py
        branchedflowsim/results/__init__.py
        branchedflowsim/results/angle_histograms.py
        branchedflowsim/results/angular_density.py
        branchedflowsim/results/caustics.py
        branchedflowsim/results/density.py
        branchedflowsim/results/trajectories.py
        branchedflowsim/results/velocity_histograms.py
        branchedflowsim/results/velocity_transitions.py
        branchedflowsim/utils/__init__.py
        branchedflowsim/utils/convenience.py
        branchedflowsim/utils/experiment.py)


set(PYTHON_TEST branchedflowsim/correlation_test.py
        branchedflowsim/medium_test.py
        branchedflowsim/potgen_test.py
        branchedflowsim/tracer_test.py
        branchedflowsim/potential_test.py
        branchedflowsim/cmdline_helpers_test.py
        branchedflowsim/io/data_spec_test.py
        branchedflowsim/io/io_test.py
        branchedflowsim/io/result_file_test.py
        branchedflowsim/test_utils.py
        branchedflowsim/observers/observer_test.py
        branchedflowsim/results/result_files_test.py
        branchedflowsim/results/empy_results_cpp_test.py)

set(CODEGEN_FILES codegen/codegen.py
        codegen/incoming_codegen.py
        codegen/observer_codegen.py
        codegen/utilities.py)

copy_file_list("${PYTHON_SOURCE}" ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
copy_file_list("${PYTHON_TEST}" ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
copy_file_list("${CODEGEN_FILES}" ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# copy the library over to the binary directory
configure_file(branchedflowsim/config.py.in ${CMAKE_CURRENT_BINARY_DIR}/branchedflowsim/config.py @ONLY )
configure_file(setup.py ${CMAKE_CURRENT_BINARY_DIR}/setup.py @ONLY )
configure_file(density_image.py ${CMAKE_CURRENT_BINARY_DIR}/density_image @ONLY)
configure_file(viewvolume.py ${CMAKE_CURRENT_BINARY_DIR}/viewvolume @ONLY)
configure_file(conf.py.in ${CMAKE_CURRENT_BINARY_DIR}/apidoc/conf.py @ONLY)

# the "ALL" here is a bit of a hack, but it ensures that we always generate the necessary python files before install
add_custom_target(generate_observer_pybinding ALL
        COMMAND python2 ${CMAKE_CURRENT_BINARY_DIR}/codegen/observer_codegen.py $<TARGET_FILE:python_glue>
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generate python binding code ${CMAKE_CURRENT_SOURCE_DIR}/codegen/observer_codegen.py"
        DEPENDS python_glue
        )

add_custom_target(generate_incoming_pybinding ALL
        COMMAND python2 ${CMAKE_CURRENT_BINARY_DIR}/codegen/incoming_codegen.py $<TARGET_FILE:python_glue>
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generate python binding code ${CMAKE_CURRENT_SOURCE_DIR}/codegen/incoming_codegen.py"
        DEPENDS python_glue
        )

# python API doc using sphinx
find_sphinx()
if(SPHINX_APIDOC)
    prepend(${CMAKE_CURRENT_BINARY_DIR} PYTHON_TEST SPHINX_EXCLUDE)
    add_apidoc_target(make_python_apidoc BranchedFlowSim "Erik Schultheis" branchedflowsim
            generate_observer_pybinding generate_incoming_pybinding)

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/apidoc/_build/html DESTINATION share/doc/branchedflowsim/python-api)
else()
    message(WARNING "Sphinx not found, no apidoc generated!")
endif()
# try pip install
find_program(PIP "pip2")

# TODO this looks wrong
if(PIP_ARGS)
  set(PIP_ARGS, "")
endif()


if (PIP)
    set(PIP_CMD "${PIP} install ${PIP_ARGS} ${CMAKE_CURRENT_BINARY_DIR}/")
        message( ${PIP_CMD})
    install(CODE "execute_process(COMMAND ${PIP_CMD})")
else()
    message(WARNING Could not find pip, python module not installed)
endif()
