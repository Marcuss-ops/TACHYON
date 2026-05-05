#include "catch2/catch_test_macros.hpp"
#include "tachyon/runtime/telemetry/render_telemetry_record.h"
#include "tachyon/runtime/telemetry/batch_telemetry_aggregator.h"
#include "tachyon/runtime/telemetry/telemetry_writer.h"
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace tachyon;

TEST_CASE("Telemetry: Retry policy for industrial errors", "[telemetry]") {
    SECTION("MissingAsset should NOT be retryable") {
        REQUIRE(is_retryable(RenderErrorCode::MissingAsset) == false);
    }
    
    SECTION("Timeout SHOULD be retryable") {
        REQUIRE(is_retryable(RenderErrorCode::Timeout) == true);
    }
    
    SECTION("EncodeFailed SHOULD be retryable") {
        REQUIRE(is_retryable(RenderErrorCode::EncodeFailed) == true);
    }
    
    SECTION("OutOfMemory should NOT be retryable") {
        REQUIRE(is_retryable(RenderErrorCode::OutOfMemory) == false);
    }
}

TEST_CASE("Telemetry: Batch aggregation and cost calculation", "[telemetry]") {
    std::vector<RenderTelemetryRecord> records;
    
    RenderTelemetryRecord r1;
    r1.run_id = "test-run";
    r1.success = true;
    r1.wall_time_ms = 1000.0; // 1 second
    r1.peak_working_set_bytes = 1024 * 1024;
    r1.avg_cpu_percent_machine = 50.0;
    r1.avg_cpu_cores_used = 4.0;
    records.push_back(r1);
    
    RenderTelemetryRecord r2;
    r2.run_id = "test-run";
    r2.success = true;
    r2.wall_time_ms = 2000.0; // 2 seconds
    r2.peak_working_set_bytes = 2048 * 1024;
    r2.avg_cpu_percent_machine = 30.0;
    r2.avg_cpu_cores_used = 2.0;
    records.push_back(r2);
    
    // Total wall time = 3000ms = 3s
    // Hourly cost = $3.6 -> $0.001 per second
    // Expected total cost = $0.003
    
    auto summary = aggregate_telemetry(records, 3.6);
    
    SECTION("Metrics are aggregated correctly") {
        REQUIRE(summary.total_jobs == 2);
        REQUIRE(summary.succeeded == 2);
        REQUIRE(summary.total_wall_time_ms == 3000.0);
        REQUIRE(summary.avg_wall_time_ms == 1500.0);
        REQUIRE(summary.peak_working_set_bytes == 2048 * 1024);
        REQUIRE(summary.avg_cpu_percent_machine == Approx(40.0));
        REQUIRE(summary.avg_cpu_cores_used == Approx(3.0));
    }
    
    SECTION("Cost is computed correctly") {
        REQUIRE(summary.machine_hourly_cost_usd == 3.6);
        REQUIRE(summary.estimated_total_cost_usd == Approx(0.003));
        REQUIRE(summary.estimated_isolated_job_cost_avg_usd == Approx(0.0015));
    }
}

TEST_CASE("Telemetry: JSONL writing safety", "[telemetry]") {
    std::string test_file = "test_telemetry_industrial.jsonl";
    if (std::filesystem::exists(test_file)) {
        std::filesystem::remove(test_file);
    }
    
    TelemetryWriter writer;
    RenderTelemetryRecord r;
    r.run_id = "run-123";
    r.job_id = "job-456";
    r.success = true;
    r.peak_private_bytes = 5000000;
    r.avg_cpu_percent_machine = 12.5;
    
    writer.append_jsonl(test_file, r);
    
    REQUIRE(std::filesystem::exists(test_file));
    
    std::ifstream ifs(test_file);
    std::string line;
    std::getline(ifs, line);
    
    SECTION("Valid JSON structure per line") {
        REQUIRE(!line.empty());
        REQUIRE(line.front() == '{');
        REQUIRE(line.back() == '}');
    }
    
    SECTION("Contains key fields") {
        REQUIRE(line.find("\"run_id\":\"run-123\"") != std::string::npos);
        REQUIRE(line.find("\"job_id\":\"job-456\"") != std::string::npos);
        REQUIRE(line.find("\"peak_private_bytes\":5000000") != std::string::npos);
        REQUIRE(line.find("\"avg_cpu_percent_machine\":12.5") != std::string::npos);
    }
    
    std::filesystem::remove(test_file);
}
