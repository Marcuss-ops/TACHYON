add_executable(tachyon-gallery-gen gallery_gen.cpp)
target_include_directories(tachyon-gallery-gen
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(tachyon-gallery-gen PRIVATE nlohmann_json::nlohmann_json)
if(MSVC)
    target_compile_options(tachyon-gallery-gen PRIVATE /W4 /utf-8)
endif()
