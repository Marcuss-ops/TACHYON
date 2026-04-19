# Core Engine Library
add_library(TachyonCore
    core/model/tachyon.cpp
    spec/parser/scene_spec.cpp
    runtime/jobs/render_job.cpp
    cli/core/cli.cpp
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
    cli/render/main.cpp
)

target_link_libraries(tachyon
    PRIVATE
        TachyonCore
)
