include_guard(GLOBAL)

if(TACHYON_USE_SYSTEM_DEPS)
    find_package(spdlog CONFIG QUIET)
endif()

if(NOT spdlog_FOUND AND NOT TARGET spdlog)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        ${TACHYON_SPDLOG_GIT_TAG}
    )
    set(SPDLOG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(SPDLOG_INSTALL        OFF CACHE BOOL "" FORCE)
    # Required when spdlog is linked into a shared library
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    FetchContent_MakeAvailable(spdlog)
endif()

function(tachyon_link_spdlog target)
    if(TARGET spdlog::spdlog)
        target_link_libraries(${target} PUBLIC spdlog::spdlog)
    elseif(TARGET spdlog)
        target_link_libraries(${target} PUBLIC spdlog)
    endif()
endfunction()
