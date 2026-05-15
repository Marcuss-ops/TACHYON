# TachyonServer module (Experimental)
option(TACHYON_BUILD_EXPERIMENTAL_SERVER "Build the experimental Media Server" OFF)

if(TACHYON_BUILD_EXPERIMENTAL_SERVER)
    message(STATUS "[Tachyon] Building experimental Media Server")
    
    set(TachyonServerSources
        ${CMAKE_CURRENT_SOURCE_DIR}/experimental/server/server.cpp
    )

    add_library(TachyonServer STATIC ${TachyonServerSources})
    tachyon_configure_common(TachyonServer)
    target_link_libraries(TachyonServer PUBLIC TachyonRuntime)
    
    target_include_directories(TachyonServer PUBLIC
        ${CMAKE_SOURCE_DIR}/include/tachyon/experimental
    )
else()
    # Create a dummy target or just skip
    add_library(TachyonServer INTERFACE)
endif()
