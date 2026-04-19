# Core Engine Library
add_library(TachyonCore
    tachyon.cpp
    scene_spec.cpp
    render_job.cpp
    cli.cpp
)

target_include_directories(TachyonCore
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(TachyonCore
    PUBLIC
        nlohmann_json::nlohmann_json
)

add_executable(tachyon
    main.cpp
)

target_link_libraries(tachyon
    PRIVATE
        TachyonCore
)
