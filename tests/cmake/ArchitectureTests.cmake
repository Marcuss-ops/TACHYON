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

add_test(
    NAME TachyonJitBoundaryCheck
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/check_jit_boundaries.py
)
tachyon_set_test_labels(TachyonJitBoundaryCheck architecture jit)

add_test(
    NAME TachyonJitAbiHeaderCheck
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/check_jit_abi_header.py
)
tachyon_set_test_labels(TachyonJitAbiHeaderCheck architecture jit)

add_test(
    NAME TachyonJitLinkDepsCheck
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/check_jit_link_deps.py
)
tachyon_set_test_labels(TachyonJitLinkDepsCheck architecture jit)
