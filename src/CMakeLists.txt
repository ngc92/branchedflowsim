include(util)
include(doxygen)

# search for all required libraries
find_package(Doxygen)
find_package(Boost 1.58.0 COMPONENTS program_options REQUIRED)

add_subdirectory(test)
add_subdirectory(common)
add_subdirectory(potgen)
add_subdirectory(tracer)

option(BUILD_DOCUMENTATION "create doxygen docs" ${DOXYGEN_FOUND})

# generate documentation
if (BUILD_DOCUMENTATION)
  SET( doxyfile             ${PROJECT_BINARY_DIR}/doxyfile )
  SET( DOXYGEN_RESULT       ${CMAKE_CURRENT_BINARY_DIR}/doc )
  SET( doxy_html_index_file ${DOXYGEN_RESULT}/html/index.html )
  SET( doxy_extra_files     ${CMAKE_CURRENT_SOURCE_DIR}/doc.dox )

  link_file_cmd(LN_CMD ${doxy_html_index_file} ${DOXYGEN_RESULT}/index.html)

  CONFIGURE_FILE(doxyfile.in ${doxyfile} @ONLY )
  ADD_CUSTOM_COMMAND( OUTPUT ${doxy_html_index_file}
					  COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
					  COMMAND ${LN_CMD}
					  MAIN_DEPENDENCY ${doxyfile} ${doxyfile_in}
					  DEPENDS common potgen tracer ${doxy_extra_files}
					  COMMENT "Generating doxygen documentation")

  ADD_CUSTOM_TARGET( doc ALL DEPENDS ${doxy_html_index_file} )
 
  # copies the doc dir to the main dir
  install(DIRECTORY ${DOXYGEN_RESULT}/ DESTINATION share/doc/branchedflowsim)
endif()
