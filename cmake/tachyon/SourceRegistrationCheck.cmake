# SourceRegistrationCheck.cmake
# Ensures all source files in specific directories are registered in the build system.

function(tachyon_check_source_registration DIRECTORY REGISTERED_SOURCES_VAR)
    file(GLOB_RECURSE ALL_SOURCES 
        "${DIRECTORY}/*.cpp"
    )
    
    foreach(SOURCE_FILE ${ALL_SOURCES})
        get_filename_component(BASE_NAME ${SOURCE_FILE} NAME)
        
        # Skip some common patterns
        if(BASE_NAME MATCHES "^\\.")
            continue()
        endif()
        
        set(FOUND FALSE)
        foreach(REG_FILE ${${REGISTERED_SOURCES_VAR}})
            get_filename_component(REG_BASE ${REG_FILE} NAME)
            if("${SOURCE_FILE}" STREQUAL "${REG_FILE}" OR "${BASE_NAME}" STREQUAL "${REG_BASE}")
                set(FOUND TRUE)
                break()
            endif()
        endforeach()
        
        if(NOT FOUND)
            message(FATAL_ERROR "Unregistered source file found: ${SOURCE_FILE}. Please add it to the appropriate CMake source list.")
        endif()
    endforeach()
endfunction()
