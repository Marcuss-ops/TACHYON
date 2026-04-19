add_executable(TachyonTests
    unit/test_main.cpp
    unit/spec/scene_spec_tests.cpp
)

target_link_libraries(TachyonTests
    PRIVATE
        TachyonCore
)
