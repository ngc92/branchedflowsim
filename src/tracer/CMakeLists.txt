set(tracer_common_SRC
	ode_state.cpp
	state.cpp
	tracer.cpp
	tracer_factory.cpp
	observers/observer.cpp
	observers/master_observer.cpp
	observers/caustic_observer.cpp
	observers/density_observer.cpp
	observers/density_worker.cpp
	observers/wavefront_observer.cpp
	observers/trajectory_observer.cpp
	observers/angular_histogram_obs.cpp
	observers/observer_factory.cpp
	initial_conditions_fwd.hpp
	initial_conditions/initial_conditions.cpp
	initial_conditions/radial_wave.cpp
	initial_conditions/planar_wave.cpp
	initial_conditions/generic_form.cpp
	initial_conditions/init_factory.cpp
	dynamics/dynamics_factory.cpp
	dynamics/ParticleInPotentialDynamics.cpp
	dynamics/ParticleInScaledPotential.cpp
	dynamics/sound.cpp
	observers/velocity_histogram_observer.cpp
	observers/velocity_histogram_observer.hpp
    observers/velocity_transition_observer.cpp
    observers/velocity_transition_observer.hpp
    observers/energy_error_observer.hpp
	observers/energy_error_observer.cpp
    observers/radial_density_observer.cpp
    observers/radial_density_observer.hpp)

set(tracer_programme_SRC
	main.cpp
	trace_args.cpp
)

set(tracer_test_SRC
    test/utils.hpp
    #	test/observer_test.cpp
    #	test/tracer_test.cpp
    test/sound_test.cpp
    test/radial_init_test.cpp
    test/init_cond_test.cpp
    test/planar_init_test.cpp
    observers/test/observer_test.cpp test/init_cond_cmdline.cpp)

add_library(tracer_common STATIC ${tracer_common_SRC})
target_include_directories(tracer_common PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tracer_common PUBLIC common PRIVATE pthread lua)


add_executable(tracer ${tracer_programme_SRC})
target_link_libraries(tracer tracer_common Boost::program_options)

add_executable(python_glue python_glue.cpp )
target_link_libraries(python_glue tracer_common)

add_executable(tracer_test ${tracer_test_SRC} )
target_link_libraries(tracer_test tracer_common test_lib)
target_compile_definitions(tracer_test PRIVATE BOOST_TEST_DYN_LINK)

add_executable(generate_observer_test_files observers/test/write_empty.cpp)
target_link_libraries(generate_observer_test_files tracer_common test_lib)
target_compile_definitions(generate_observer_test_files PRIVATE BOOST_TEST_DYN_LINK)

install(TARGETS tracer RUNTIME DESTINATION ${BIN_INSTALL_DIR})
install(TARGETS tracer_test RUNTIME DESTINATION ${TEST_INSTALL_DIR})
install(TARGETS generate_observer_test_files RUNTIME DESTINATION ${TEST_INSTALL_DIR})
