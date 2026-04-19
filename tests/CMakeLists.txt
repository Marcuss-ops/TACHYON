add_executable(TachyonTests
    test_main.cpp
    scene_spec_tests.cpp
)

target_link_libraries(TachyonTests
    PRIVATE
        TachyonCore
)
