include_guard(GLOBAL)

if(TACHYON_FETCH_DEPS)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        ${TACHYON_SPDLOG_GIT_TAG}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(SPDLOG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(SPDLOG_INSTALL        OFF CACHE BOOL "" FORCE)
    # Required when spdlog is linked into a shared library
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    FetchContent_MakeAvailable(spdlog)
else()
    find_package(spdlog CONFIG REQUIRED)
endif()

function(tachyon_link_spdlog target)
    if(TARGET spdlog::spdlog)
        target_link_libraries(${target} PUBLIC spdlog::spdlog)
    elseif(TARGET spdlog)
        target_link_libraries(${target} PUBLIC spdlog)
    endif()
endfunction()
