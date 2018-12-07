# make config.h
set(FFTW_WISDOM_DIRECTORY ${CMAKE_INSTALL_PREFIX}/etc/branchedflowsim/)
set(FFTW_WISDOM_FILENAME ${FFTW_WISDOM_DIRECTORY}/fftw_wisdom)
set(TEST_DATA_DIRECTORY ${CMAKE_INSTALL_PREFIX}/etc/branchedflowsim/test)
# TODO What does this command do?
install(DIRECTORY DESTINATION ${FFTW_WISDOM_DIRECTORY})
configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(potgen_common_SRC
	potgen.cpp
	fft.cpp
	correlation.cpp
	discretize.cpp
	randomize.cpp
	randomize.hpp)

set(potgen_programme_SRC
	potgen_main.cpp
	potgen_args.cpp
)

set(potgen_test_SRC
	test/binary_cmp.cpp
	test/potgen_test.cpp
	test/potgen_internals_test.cpp
	test/potgen_fn_test.cpp
	test/fft_test.cpp
	test/derivatives_test.cpp
	test/discretize_test.cpp
)


find_package(FFTW REQUIRED)

add_library(potgen_common STATIC ${potgen_common_SRC})
target_include_directories(potgen_common PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(potgen_common PUBLIC common PRIVATE fftw::fftw lua::lua pthread)


add_executable(potgen ${potgen_programme_SRC})
target_link_libraries(potgen PRIVATE Boost::program_options potgen_common)

add_executable(potgen_test ${potgen_test_SRC} )
target_link_libraries(potgen_test PRIVATE test_lib potgen_common)
target_compile_definitions(potgen_test PRIVATE BOOST_TEST_DYN_LINK)


install(TARGETS potgen RUNTIME DESTINATION ${BIN_INSTALL_DIR})
install(TARGETS potgen_test RUNTIME DESTINATION ${TEST_INSTALL_DIR})
install(FILES ${PROJECT_SOURCE_DIR}/test/pot_2d_128_ref DESTINATION etc/branchedflowsim/test)
