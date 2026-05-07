# ---------------------------------------------------------
# 0. ARCHITECTURE DOMAIN: TachyonArchitectureTests
# boundary checks for neutral spec/preset/scene-eval layers
# ---------------------------------------------------------
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_test(
    NAME TachyonArchitectureBoundaryCheck
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/check_arch_boundaries.py
)
tachyon_set_test_labels(TachyonArchitectureBoundaryCheck architecture)
