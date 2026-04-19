include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee9d7daed91c74a4c21946c180da388d.zip
)
# For Windows: prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Define the test runner
add_executable(TachyonTests test_main.cpp)

# target_sources(TachyonTests PRIVATE test_main.cpp)

target_link_libraries(TachyonTests 
    PRIVATE 
        TachyonCore
        GTest::gtest_main
)

# Discover tests
include(GoogleTest)
gtest_discover_tests(TachyonTests)
