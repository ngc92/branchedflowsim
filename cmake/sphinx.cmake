function(find_sphinx)
    find_program(SPHINX_APIDOC sphinx-apidoc)
endfunction()

function(add_apidoc_target TARGET TITLE AUTHOR SOURCE)
    find_sphinx()
    add_custom_target(${TARGET}_prepare ALL
            COMMAND sphinx-apidoc -F -H "${TITLE}" -A "${AUTHOR}" -o apidoc ${SOURCE} ${SPHINX_EXCLUDE}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Preparing python API docs"
            DEPENDS ${ARGN}
            VERBATIM
            )

    add_custom_target(${TARGET} ALL
            COMMAND make html
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/apidoc
            COMMENT "Generate python API docs"
            DEPENDS ${TARGET}_prepare
            )

endfunction()