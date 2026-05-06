include_guard(GLOBAL)

if(TACHYON_ENABLE_3D)
    if(TACHYON_FETCH_DEPS)
        FetchContent_Declare(
            embree
            GIT_REPOSITORY https://github.com/embree/embree.git
            GIT_TAG        ${TACHYON_EMBREE_GIT_TAG}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        set(EMBREE_ISPC_SUPPORT OFF CACHE BOOL "Disable ISPC for Embree" FORCE)
        set(EMBREE_TUTORIALS OFF CACHE BOOL "Disable Embree tutorials" FORCE)
        set(EMBREE_TASKING_SYSTEM INTERNAL CACHE STRING "Use internal tasking for Embree" FORCE)
        FetchContent_MakeAvailable(embree)
    else()
        find_package(embree 3.13 REQUIRED)
    endif()
    set(EMBREE_STATIC_LIB ON CACHE BOOL "Build Embree as static library" FORCE)
    set(EMBREE_RAY_MASK ON CACHE BOOL "Enable Embree ray masking" FORCE)
    set(EMBREE_FILTER_FUNCTION ON CACHE BOOL "Enable Embree filter functions" FORCE)
    set(EMBREE_TASKING_SYSTEM INTERNAL CACHE STRING "Use internal tasking system for Embree" FORCE)
    set(EMBREE_MAX_ISA "SSE4.2" CACHE STRING "Max ISA for Embree" FORCE)
else()
    message(STATUS "[Tachyon] 3D feature disabled; skipping Embree fetch")
endif()
