include_guard(GLOBAL)

if(TACHYON_FETCH_DEPS)
    FetchContent_Declare(
        cpu_features
        GIT_REPOSITORY https://github.com/google/cpu_features.git
        GIT_TAG        ${TACHYON_CPU_FEATURES_GIT_TAG}
    )
    
    # Disable building tests and executables for cpu_features
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(CPU_FEATURES_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(cpu_features)
endif()
