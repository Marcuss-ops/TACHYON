#include "test_utils.h"
#include "tachyon/runtime/telemetry/thread_local_telemetry.h"
#include <iostream>
#include <thread>
#include <cassert>

using namespace tachyon::runtime;

bool run_thread_local_telemetry_tests() {
    std::cout << "[ThreadLocalTelemetryTests] Starting ThreadLocalTelemetry unit tests...\n";

    // Test 1: Thread-local isolation
    {
        tl_telemetry.reset();
        tl_telemetry.pixel_ops = 1000;
        tl_telemetry.rasterized_pixels = 800;
        tl_telemetry.tiles_processed = 10;

        std::thread t1([]() {
            tl_telemetry.reset();
            if (tl_telemetry.pixel_ops != 0 || tl_telemetry.tiles_processed != 0) {
                std::cerr << "[ThreadLocalTelemetryTests] FAIL: Thread-local state not isolated initially\n";
            }
            tl_telemetry.pixel_ops = 5000;
            tl_telemetry.blend_pixel_ops = 2000;
            tl_telemetry.tiles_processed = 50;
        });
        t1.join();

        if (tl_telemetry.pixel_ops != 1000 || tl_telemetry.rasterized_pixels != 800 || tl_telemetry.tiles_processed != 10) {
            std::cerr << "[ThreadLocalTelemetryTests] FAIL: Main thread telemetry was modified by background thread\n";
            return false;
        }

        std::cout << "[ThreadLocalTelemetryTests] Thread isolation verified!\n";
    }

    // Test 2: Flushing behavior
    {
        tl_telemetry.reset();
        tl_telemetry.pixel_ops = 2000;
        tl_telemetry.rasterized_pixels = 1500;
        tl_telemetry.blend_pixel_ops = 500;
        tl_telemetry.encoded_pixels = 1000;
        tl_telemetry.tiles_processed = 20;

        auto total_ops = std::make_shared<std::atomic<std::size_t>>(10000);
        auto total_raster = std::make_shared<std::atomic<std::size_t>>(5000);
        auto total_blend = std::make_shared<std::atomic<std::size_t>>(1000);
        auto total_encoded = std::make_shared<std::atomic<std::size_t>>(2000);
        auto total_tiles = std::make_shared<std::atomic<int>>(100);

        flush_thread_local_telemetry(total_ops, total_tiles, total_raster, total_blend, total_encoded);

        if (total_ops->load() != 12000) {
            std::cerr << "[ThreadLocalTelemetryTests] FAIL: Pixels ops flush failed. Got " << total_ops->load() << "\n";
            return false;
        }

        if (total_raster->load() != 6500) {
            std::cerr << "[ThreadLocalTelemetryTests] FAIL: Rasterized pixels flush failed. Got " << total_raster->load() << "\n";
            return false;
        }

        if (total_blend->load() != 1500) {
            std::cerr << "[ThreadLocalTelemetryTests] FAIL: Blend pixels flush failed. Got " << total_blend->load() << "\n";
            return false;
        }

        if (total_encoded->load() != 3000) {
            std::cerr << "[ThreadLocalTelemetryTests] FAIL: Encoded pixels flush failed. Got " << total_encoded->load() << "\n";
            return false;
        }

        if (tl_telemetry.pixel_ops != 0 || tl_telemetry.tiles_processed != 0) {
            std::cerr << "[ThreadLocalTelemetryTests] FAIL: Telemetry was not reset after flush\n";
            return false;
        }

        std::cout << "[ThreadLocalTelemetryTests] Flushing and resetting verified!\n";
    }

    std::cout << "[ThreadLocalTelemetryTests] ALL TESTS PASSED!\n";
    return true;
}
