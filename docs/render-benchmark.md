# Tachyon Render Benchmark Tool

The `tachyon-render-benchmark` is a native, standalone, external benchmarking tool that runs real end-to-end rendering loops via the `tachyon` command-line interface.

It automates execution, timing measurement, variant comparison, statistics calculation, and file verification, exporting detailed summaries as ASCII tables, CSV files, and JSON documents.

---

## 1. Features
- **Variant Comparison**: Compares five distinct performance configurations of the rendering engine:
  1. `baseline`: Standard software encoding and synchronous frame outputs.
  2. `async_output`: Overlapped frame writing using asynchronous worker queue.
  3. `hardware_encoder`: Automatic discovery and utilization of hardware-accelerated video encoders.
  4. `hardware_encoder_async_output`: Joint hardware acceleration and asynchronous pipelines.
  5. `telemetry_enabled`: Measuring telemetry SQLite writes and logging overhead.
- **Robust Subprocess Management**: Runs the main `tachyon` binary as an external subprocess, configuring environment variables on-the-fly.
- **Rich Performance Statistics**:
  - Minimum, maximum, median wall-clock time (in milliseconds).
  - 95th percentile (P95) execution times.
  - Effective FPS (Frames Per Second).
  - Performance speedup factor relative to baseline.
  - Successful execution rates.
- **Data Export**: Generates raw timing logs in `results.csv` and comprehensive aggregated metadata + run-by-run logs in structured `results.json`.

---

## 2. Compilation

The benchmark tool is integrated into the CMake build tree under the `tools` directory.

To build the executable:
```bash
# Build the benchmark binary
cmake --build build --target tachyon-render-benchmark
```
This produces the executable binary at `build/tools/render_benchmark/tachyon-render-benchmark`.

---

## 3. Usage

The tool accepts flexible CLI options to target different inputs and iteration lengths:

```bash
./build/tools/render_benchmark/tachyon-render-benchmark [OPTIONS]
```

### Options:
- `--tachyon <path>`: Path to the `tachyon` CLI binary (e.g. `./build/src/tachyon`).
- `--scene <path>`: Path to a JIT C++ scene file (e.g. `benchmark_scene.cpp`).
- `--preset <id>`: Built-in preset name to render.
- `--frames <range>`: Frame range to render (e.g. `0-10`, `0-179`). Default is `0-179`.
- `--runs <n>`: Number of measured runs per variant (default: `5`).
- `--warmup <n>`: Number of warmup runs discarded before measurements (default: `1`).
- `--out-dir <path>`: Folder for intermediate MP4 files (default: `benchmark_out`).
- `--json <path>`: Destination path for the JSON report (default: `<out-dir>/results.json`).
- `--csv <path>`: Destination path for the CSV report (default: `<out-dir>/results.csv`).

---

## 4. Example Invocation

To benchmark the JIT dynamic compiled C++ scene:
```bash
./build/tools/render_benchmark/tachyon-render-benchmark \
    --tachyon ./build/src/tachyon \
    --scene benchmark_scene.cpp \
    --frames 0-100 \
    --runs 3 \
    --warmup 1 \
    --out-dir benchmark_out
```

Upon completion, it prints a beautifully aligned summary table:

```text
==========================================================================================
                               BENCHMARK COMPREHENSIVE SUMMARY
==========================================================================================
Variant                          Median (ms)  Min (ms)  Max (ms)  P95 (ms)       FPS   SpeedupSuccess Rate
------------------------------------------------------------------------------------------
baseline                              1561.8    1561.4    1562.1    1562.1      7.04     1.00x     100.00%
async_output                          1569.5    1567.5    1571.5    1571.5      7.01     1.00x     100.00%
hardware_encoder                      1570.7    1568.9    1572.5    1572.5      7.00     0.99x     100.00%
hardware_encoder_async_output         1560.9    1558.9    1562.9    1562.9      7.05     1.00x     100.00%
telemetry_enabled                     1571.9    1567.6    1576.2    1576.2      7.00     0.99x     100.00%
==========================================================================================
```
