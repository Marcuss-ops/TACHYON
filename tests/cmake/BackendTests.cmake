# ---------------------------------------------------------
# BACKEND DOMAIN: TachyonBackendTests
# Registry, capability dispatch, hardware backends
# ---------------------------------------------------------
add_executable(TachyonBackendTests
    unit/mains/test_main_backend.cpp
    unit/backends/registry_tests.cpp
)

target_link_libraries(TachyonBackendTests
    PRIVATE
        TachyonTestUtils
        TachyonBackends
)

add_test(
    NAME TachyonBackendTests
    COMMAND TachyonBackendTests
)
tachyon_set_test_labels(TachyonBackendTests backend)
