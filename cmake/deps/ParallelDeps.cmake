include_guard(GLOBAL)

if(TACHYON_ENABLE_TASKFLOW)
    if(TACHYON_USE_SYSTEM_DEPS)
        find_package(Taskflow CONFIG QUIET)
    endif()

    if(NOT Taskflow_FOUND AND NOT TARGET Taskflow)
        FetchContent_Declare(
            taskflow
            GIT_REPOSITORY https://github.com/taskflow/taskflow.git
            GIT_TAG ${TACHYON_TASKFLOW_GIT_TAG}
        )
        set(TF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(TF_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(taskflow)
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
