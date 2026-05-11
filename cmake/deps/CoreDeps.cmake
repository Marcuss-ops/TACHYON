include_guard(GLOBAL)

if(TACHYON_FETCH_DEPS)
    # Clipper2 for boolean shape operations (Merge Paths)
    set(CLIPPER2_UTILS OFF CACHE BOOL "Disable Clipper2 utils" FORCE)
    set(CLIPPER2_EXAMPLES OFF CACHE BOOL "Disable Clipper2 examples" FORCE)
    set(CLIPPER2_TESTS OFF CACHE BOOL "Disable Clipper2 tests" FORCE)
    
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/clipper2/CPP/CMakeLists.txt")
        set(clipper2_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/clipper2")
        set(clipper2_BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/clipper2-build")
        add_subdirectory("${clipper2_SOURCE_DIR}/CPP" "${clipper2_BINARY_DIR}" EXCLUDE_FROM_ALL)
    else()
        FetchContent_Declare(
            clipper2
            GIT_REPOSITORY https://github.com/AngusJohnson/Clipper2.git
            GIT_TAG        ${TACHYON_CLIPPER2_GIT_TAG}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(clipper2)
    endif()

    # GoogleTest for unit testing
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/googletest/CMakeLists.txt")
        set(googletest_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/googletest")
        set(googletest_BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/googletest-build")
        add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
    else()
        FetchContent_Declare(
            googletest
            URL ${TACHYON_GTEST_URL}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        # For Windows: prevent gtest from overriding our compiler/linker options
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    # Abseil for high-performance data structures and logging
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/abseil/CMakeLists.txt")
        set(absl_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/abseil")
        set(absl_BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/abseil-build")
        add_subdirectory(${absl_SOURCE_DIR} ${absl_BINARY_DIR} EXCLUDE_FROM_ALL)
    else()
        FetchContent_Declare(
            absl
            GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
            GIT_TAG        20240116.2 # LTS 2024
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(absl)
    endif()
endif()
