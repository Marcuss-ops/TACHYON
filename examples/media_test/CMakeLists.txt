# Tachyon Media Test
add_executable(tachyon-media-test main.cpp)
target_link_libraries(tachyon-media-test PRIVATE TachyonMediaOps TachyonCore TachyonRuntime TachyonCatalog)
