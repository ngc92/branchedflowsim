function(link_file_cmd ln_command source target)
    if(UNIX)
        SET( ${ln_command} ln -sfr ${source} ${target} PARENT_SCOPE)
    else()
        SET( ${ln_command} \"${target}\" \"${source}\" PARENT_SCOPE)
    endif()
endfunction(link_file_cmd)