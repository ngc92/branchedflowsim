set(common_test_SRC
        fileio_test.cpp
        global_test.cpp
        caustic_test.cpp
        linear_interpolation_test.cpp
        potential_test.cpp
        dynamic_grid_test.cpp
        multiindex_test.cpp
	argspec_test.cpp
        argset_test.cpp
        argval_test.cpp
		args_usage_test.cpp factory_test.cpp)

add_executable(common_test ${common_test_SRC} )
target_link_libraries(common_test test_lib common ${Boost_LIBRARIES} lua test_lib)
target_compile_definitions(common_test PRIVATE BOOST_TEST_DYN_LINK)
install(TARGETS common_test RUNTIME DESTINATION ${TEST_INSTALL_DIR})
