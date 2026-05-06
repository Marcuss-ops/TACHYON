# Tachyon Compiler Cache Configuration

if(TACHYON_ENABLE_COMPILER_CACHE)
    find_program(TACHYON_COMPILER_LAUNCHER NAMES sccache ccache clcache)
    if(TACHYON_COMPILER_LAUNCHER)
        message(STATUS "Using compiler launcher: ${TACHYON_COMPILER_LAUNCHER}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${TACHYON_COMPILER_LAUNCHER}" CACHE FILEPATH "Compiler launcher" FORCE)
        set(CMAKE_C_COMPILER_LAUNCHER "${TACHYON_COMPILER_LAUNCHER}" CACHE FILEPATH "Compiler launcher" FORCE)
    endif()
endif()
