cmake_minimum_required(VERSION 3.5)   # require at least 3.5 - needed for boost imported targets.

project(branchedflowsim VERSION 0.1.0)

# TODO profile build

# these warnings come from boost. We can't do anything about them, so we disable them.
add_compile_options(-Wno-unused-local-typedefs)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(BIN_INSTALL_DIR bin)
set(TEST_INSTALL_DIR bin/test)

# more config for cmake
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# plug the README.md into doxygen
configure_file(README.md src/README.md)

add_subdirectory(deps)
add_subdirectory(src)
add_subdirectory(python)
