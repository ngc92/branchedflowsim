set(common_SRC
        caustic.cpp
        fileIO.cpp
        potential.cpp
        profiling.cpp
        multiindex.cpp
        grid_storage.cpp
        interpolation.cpp
        dynamic_grid_base.cpp
        util.hpp
        vector.hpp
        global.hpp
        dynamic_grid_base.hpp
        dynamic_grid.hpp
        args/spec.hpp
        args/argument_set.hpp
        args/argument_set.cpp
        args/usage.hpp
        args/usage.cpp
        args/value.hpp
        args/value.cpp
        args/error.hpp
        args/error.cpp
        args/spec.cpp
        factory/builder_base.hpp
        factory/builder_base.cpp
        factory/factory.hpp
        factory/factory.cpp)

add_subdirectory(test)
add_library(common ${common_SRC})
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(common PUBLIC Boost::boost)
