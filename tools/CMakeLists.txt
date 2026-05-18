# Tachyon Tools
add_subdirectory(media_stress_test)
add_subdirectory(render_benchmark)

# Telemetry Seeder
add_executable(tachyon_seed_telemetry telemetry/seed_demo_database.cpp)
tachyon_configure_common(tachyon_seed_telemetry)
target_link_libraries(tachyon_seed_telemetry PRIVATE TachyonRuntime)

