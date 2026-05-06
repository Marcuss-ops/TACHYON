#include <gtest/gtest.h>
#include "tachyon/runtime/telemetry/render_telemetry_record.h"
#include "tachyon/runtime/telemetry/batch_telemetry_aggregator.h"
#include "tachyon/runtime/telemetry/telemetry_writer.h"
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace tachyon;

TEST(TelemetryIndustrialTests, RetryPolicyForIndustrialErrors) {
    EXPECT_FALSE(is_retryable(RenderErrorCode::MissingAsset));
    EXPECT_TRUE(is_retryable(RenderErrorCode::Timeout));
    EXPECT_TRUE(is_retryable(RenderErrorCode::EncodeFailed));
    EXPECT_FALSE(is_retryable(RenderErrorCode::OutOfMemory));
}

TEST(TelemetryIndustrialTests, BatchAggregationAndCostCalculation) {
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
    
    auto summary = aggregate_telemetry(records, 3.6);
    
    EXPECT_EQ(summary.total_jobs, 2);
    EXPECT_EQ(summary.succeeded, 2);
    EXPECT_EQ(summary.total_wall_time_ms, 3000.0);
    EXPECT_EQ(summary.avg_wall_time_ms, 1500.0);
    EXPECT_EQ(summary.peak_working_set_bytes, 2048 * 1024);
    EXPECT_NEAR(summary.avg_cpu_percent_machine, 40.0, 0.001);
    EXPECT_NEAR(summary.avg_cpu_cores_used, 3.0, 0.001);
    
    EXPECT_EQ(summary.machine_hourly_cost_usd, 3.6);
    EXPECT_NEAR(summary.estimated_total_cost_usd, 0.003, 0.0001);
    EXPECT_NEAR(summary.estimated_isolated_job_cost_avg_usd, 0.0015, 0.0001);
}

TEST(TelemetryIndustrialTests, JsonlWritingSafety) {
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
    
    TelemetryWriter::append_jsonl(r, test_file);
    
    EXPECT_TRUE(std::filesystem::exists(test_file));
    
    std::ifstream ifs(test_file);
    std::string line;
    std::getline(ifs, line);
    
    EXPECT_FALSE(line.empty());
    EXPECT_EQ(line.front(), '{');
    EXPECT_EQ(line.back(), '}');
    
    EXPECT_NE(line.find("\"run_id\":\"run-123\""), std::string::npos);
    EXPECT_NE(line.find("\"job_id\":\"job-456\""), std::string::npos);
    EXPECT_NE(line.find("\"peak_private_bytes\":5000000"), std::string::npos);
    EXPECT_NE(line.find("\"avg_cpu_percent_machine\":12.5"), std::string::npos);
    
    std::filesystem::remove(test_file);
}

bool run_telemetry_industrial_tests() {
    std::cout << "[run] telemetry_industrial_tests\n";
    TelemetryIndustrialTests_RetryPolicyForIndustrialErrors();
    TelemetryIndustrialTests_BatchAggregationAndCostCalculation();
    TelemetryIndustrialTests_JsonlWritingSafety();
    return true;
}
