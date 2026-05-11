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
