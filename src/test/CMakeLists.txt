set(test_common_SRC
        binary_cmp.cpp
        test_helpers.cpp
        )

find_package(Boost COMPONENTS unit_test_framework REQUIRED)

add_library(test_lib ${test_common_SRC})
target_include_directories(test_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_lib common Boost::unit_test_framework)
