include_guard(GLOBAL)



if(TACHYON_ENABLE_OUTPUT)
    if(TACHYON_FETCH_DEPS)
        if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/tinyexr/CMakeLists.txt")
            add_subdirectory("${CMAKE_SOURCE_DIR}/third_party/tinyexr" "${CMAKE_BINARY_DIR}/_deps/tinyexr-build" EXCLUDE_FROM_ALL)
        else()
            FetchContent_Declare(
                tinyexr
                GIT_REPOSITORY https://github.com/syoyo/tinyexr.git
                GIT_TAG        ${TACHYON_TINYEXR_GIT_TAG}
            )
            FetchContent_MakeAvailable(tinyexr)
        endif()
    endif()
else()
    message(STATUS "[Tachyon] Output feature disabled; skipping tinyexr fetch")
endif()

function(tachyon_link_output_deps target)
    if(TACHYON_ENABLE_OUTPUT)
        target_compile_definitions(${target} PUBLIC TACHYON_ENABLE_OUTPUT)
        if(TACHYON_ENABLE_OIDN AND WIN32)
            target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${oidn_SOURCE_DIR}/include>)
            target_link_libraries(${target} PUBLIC OpenImageDenoise)
        endif()
    endif()
endfunction()
