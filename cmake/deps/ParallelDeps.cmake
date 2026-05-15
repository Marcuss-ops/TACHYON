include_guard(GLOBAL)

if(TACHYON_ENABLE_TASKFLOW)
    if(TACHYON_FETCH_DEPS)
        FetchContent_Declare(
            taskflow
            GIT_REPOSITORY https://github.com/taskflow/taskflow.git
            GIT_TAG ${TACHYON_TASKFLOW_GIT_TAG}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(taskflow)
    else()
        find_package(Taskflow CONFIG REQUIRED)
    endif()
endif()

function(tachyon_link_omp target)
    if(TACHYON_HAS_OPENMP AND TARGET OpenMP::OpenMP_CXX)
        target_link_libraries(${target} PUBLIC OpenMP::OpenMP_CXX)
    endif()
endfunction()
function(tachyon_link_taskflow target)
    if(TACHYON_ENABLE_TASKFLOW)
        if(TARGET Taskflow)
            target_link_libraries(${target} PRIVATE Taskflow)
        elseif(TARGET taskflow)
            target_link_libraries(${target} PRIVATE taskflow)
        elseif(TARGET Taskflow::Taskflow)
            target_link_libraries(${target} PRIVATE Taskflow::Taskflow)
        endif()
        target_compile_definitions(${target} PRIVATE TACHYON_ENABLE_TASKFLOW)
    endif()
endfunction()
