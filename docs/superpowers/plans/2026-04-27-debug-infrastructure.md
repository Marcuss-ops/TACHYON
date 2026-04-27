# Debug Infrastructure Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Portare Tachyon a livello di debugging dei grandi studi: logging strutturato, assert informativi, profiler thread-safe, e integrazione Tracy opzionale.

**Architecture:** Un header `logging.h` centralizza spdlog; un header `assert.h` fornisce `TACHYON_ASSERT`; il `ProfilerData` viene reso thread-safe con `thread_local` + mutex; Tracy viene aggiunto come opzione CMake off-by-default. Tutti i `fprintf(stderr)` e `std::cerr` interni vengono migrati ai nuovi macro. La CLI (`cli.cpp`) usa `std::cout`/`std::cerr` intenzionalmente per output utente — non va toccata.

**Tech Stack:** C++20, spdlog 1.14.1 (header-only via FetchContent), Tracy 0.11.1 (opzionale, via FetchContent), GoogleTest (già presente)

---

## File Map

| Azione | File | Responsabilità |
|--------|------|----------------|
| Create | `include/tachyon/core/logging.h` | Macro `TLOG_*`, inizializzazione logger |
| Create | `include/tachyon/core/assert.h` | Macro `TACHYON_ASSERT` e `TACHYON_ASSERT_ALWAYS` |
| Create | `include/tachyon/core/profiling.h` | Macro `TACHYON_ZONE` (Tracy o no-op) |
| Create | `tests/unit/core/logging_tests.cpp` | Test output catturato su ostream sink |
| Create | `tests/unit/core/assert_tests.cpp` | Test assert non-aborting |
| Create | `tests/unit/renderer3d/profiler_tests.cpp` | Test thread-safety profiler |
| Modify | `CMakeLists.txt` | FetchContent spdlog + Tracy option |
| Modify | `src/CMakeLists.txt` | `target_link_libraries` spdlog |
| Modify | `include/tachyon/renderer3d/utilities/debug_tools.h` | Rimuovi `section_start_`/`current_section_`, aggiungi mutex |
| Modify | `src/renderer3d/utilities/debug_tools.cpp` | `thread_local` starts, lock_guard |
| Modify | `src/audio/audio_decoder.cpp` | `fprintf(stderr)` → `TLOG_ERROR` |
| Modify | `src/audio/audio_encoder.cpp` | `fprintf(stderr)` → `TLOG_ERROR` |
| Modify | `src/audio/audio_export.cpp` | `std::cout` → `TLOG_INFO` |
| Modify | `src/debug/debug_export.cpp` | `std::cerr` → `TLOG_*` |

---

## Task 1: Aggiungere spdlog via FetchContent

**Files:**
- Modify: `CMakeLists.txt` (dopo il blocco FetchContent di googletest, prima di `add_subdirectory(src)`)

- [ ] **Step 1: Aggiungere spdlog FetchContent in CMakeLists.txt**

Aprire `CMakeLists.txt`. Dopo il blocco `FetchContent_MakeAvailable(googletest)` (riga ~200), aggiungere:

```cmake
# spdlog — structured logging (header-only)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(SPDLOG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spdlog)
```

- [ ] **Step 2: Linkare spdlog a TachyonCore in src/CMakeLists.txt**

In `src/CMakeLists.txt`, nel blocco `target_link_libraries(TachyonCore PUBLIC ...)` (riga ~281), aggiungere `spdlog::spdlog` tra le dipendenze PUBLIC:

```cmake
target_link_libraries(TachyonCore
    PUBLIC
        nlohmann_json::nlohmann_json
        freetype
        harfbuzz
        OpenMP::OpenMP_CXX
        spdlog::spdlog
)
```

- [ ] **Step 3: Verificare che CMake configuri senza errori**

```
cmake --preset relwithdebinfo
```

Output atteso: `-- Configuring done` senza errori. Ci sarà una nuova riga tipo `-- spdlog: ...`.

- [ ] **Step 4: Commit**

```bash
git add CMakeLists.txt src/CMakeLists.txt
git commit -m "build: add spdlog 1.14.1 via FetchContent"
```

---

## Task 2: Creare include/tachyon/core/logging.h

**Files:**
- Create: `include/tachyon/core/logging.h`

- [ ] **Step 1: Scrivere il test che verifica il logging**

Creare `tests/unit/core/logging_tests.cpp`:

```cpp
#include <gtest/gtest.h>
#include "tachyon/core/logging.h"
#include <spdlog/sinks/ostream_sink.h>
#include <sstream>

namespace {

std::string capture_log(spdlog::level::level_enum level, std::function<void()> fn) {
    std::ostringstream oss;
    auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
    sink->set_level(spdlog::level::trace);
    sink->set_pattern("%l %v");

    auto test_logger = std::make_shared<spdlog::logger>("test_capture", sink);
    test_logger->set_level(spdlog::level::trace);

    auto prev = spdlog::get("tachyon");
    spdlog::register_logger(test_logger);
    // Swap the global tachyon logger temporarily
    auto real = tachyon::detail::swap_logger_for_test(test_logger);
    fn();
    tachyon::detail::swap_logger_for_test(real);
    spdlog::drop("test_capture");

    return oss.str();
}

} // namespace

TEST(Logging, InfoMessageAppearsInOutput) {
    std::ostringstream oss;
    auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
    sink->set_level(spdlog::level::trace);
    sink->set_pattern("%v");

    auto logger = std::make_shared<spdlog::logger>("tachyon_test", sink);
    logger->set_level(spdlog::level::trace);

    logger->info("hello logging");
    EXPECT_NE(oss.str().find("hello logging"), std::string::npos);
}

TEST(Logging, LoggerSingletonIsNotNull) {
    EXPECT_NE(tachyon::get_logger(), nullptr);
}

TEST(Logging, MacroDoesNotCrash) {
    TLOG_INFO("test message {}", 42);
    TLOG_WARN("warn {}", "x");
    TLOG_ERROR("error {}", 0);
    // If we get here, no crash
    SUCCEED();
}
```

- [ ] **Step 2: Verificare che il test NON compili (logging.h non esiste ancora)**

```
cmake --build build-ninja --target TachyonTests 2>&1 | grep "logging_tests"
```

Output atteso: errore di compilazione su `logging_tests.cpp`.

- [ ] **Step 3: Creare include/tachyon/core/logging.h**

```cpp
#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>

namespace tachyon {

inline std::shared_ptr<spdlog::logger>& get_logger() {
    static std::shared_ptr<spdlog::logger> logger = [] {
        auto l = spdlog::stdout_color_mt("tachyon");
        l->set_level(spdlog::level::warn);
        l->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] [t%t] %v");
        return l;
    }();
    return logger;
}

namespace detail {
inline std::shared_ptr<spdlog::logger> swap_logger_for_test(
        std::shared_ptr<spdlog::logger> replacement) {
    auto prev = get_logger();
    get_logger() = std::move(replacement);
    return prev;
}
} // namespace detail

} // namespace tachyon

// Livello controllabile via TACHYON_LOG_LEVEL a compile time o runtime
#define TLOG_TRACE(...)    ::tachyon::get_logger()->trace(__VA_ARGS__)
#define TLOG_DEBUG(...)    ::tachyon::get_logger()->debug(__VA_ARGS__)
#define TLOG_INFO(...)     ::tachyon::get_logger()->info(__VA_ARGS__)
#define TLOG_WARN(...)     ::tachyon::get_logger()->warn(__VA_ARGS__)
#define TLOG_ERROR(...)    ::tachyon::get_logger()->error(__VA_ARGS__)
#define TLOG_CRITICAL(...) ::tachyon::get_logger()->critical(__VA_ARGS__)
```

- [ ] **Step 4: Aggiungere il test file al CMakeLists.txt dei test**

In `tests/unit/core/CMakeLists.txt` (o nel file che lista i sorgenti del target TachyonTests), aggiungere `logging_tests.cpp`.

Prima controllare come sono listati i file esistenti:
```bash
grep -n "math_tests\|expression_tests" tests/unit/core/CMakeLists.txt
```
Aggiungere sulla stessa riga o nella stessa lista: `logging_tests.cpp`.

- [ ] **Step 5: Build e verificare che i test passino**

```
cmake --build build-ninja --target TachyonTests
build-ninja\tests\unit\TachyonTests.exe --gtest_filter="Logging*"
```

Output atteso:
```
[  PASSED  ] 3 tests.
```

- [ ] **Step 6: Commit**

```bash
git add include/tachyon/core/logging.h tests/unit/core/logging_tests.cpp tests/unit/core/CMakeLists.txt
git commit -m "feat: add spdlog-based structured logging (TLOG_* macros)"
```

---

## Task 3: Creare include/tachyon/core/assert.h

**Files:**
- Create: `include/tachyon/core/assert.h`
- Create: `tests/unit/core/assert_tests.cpp`

- [ ] **Step 1: Scrivere il test per le assert**

Creare `tests/unit/core/assert_tests.cpp`:

```cpp
#include <gtest/gtest.h>
#include "tachyon/core/assert.h"

TEST(Assert, TrueConditionDoesNotAbort) {
    // Se TACHYON_ASSERT su condizione vera causa problemi, il test crasha
    TACHYON_ASSERT(1 == 1, "one equals one");
    TACHYON_ASSERT(true, "true is true");
    SUCCEED();
}

TEST(Assert, AlwaysAssertTrueDoesNotAbort) {
    TACHYON_ASSERT_ALWAYS(2 + 2 == 4, "math works");
    SUCCEED();
}

// Nota: non testiamo TACHYON_ASSERT(false, ...) perché chiama abort().
// In produzione lo si verifica con death test:
// EXPECT_DEATH(TACHYON_ASSERT_ALWAYS(false, "boom"), "ASSERT FAILED");
// Ma death tests sono lenti — lasciamo fuori dal CI normale.
```

- [ ] **Step 2: Creare include/tachyon/core/assert.h**

```cpp
#pragma once

#include "tachyon/core/logging.h"
#include <cstdlib>

// TACHYON_ASSERT — attivo solo in debug (NDEBUG non definita)
// TACHYON_ASSERT_ALWAYS — attivo sempre, anche in release
#define TACHYON_ASSERT_ALWAYS(cond, msg)                                   \
    do {                                                                    \
        if (!(cond)) {                                                      \
            TLOG_CRITICAL("ASSERT FAILED: ({}) | {} | {}:{}",              \
                          #cond, msg, __FILE__, __LINE__);                  \
            ::tachyon::get_logger()->flush();                               \
            std::abort();                                                   \
        }                                                                   \
    } while (0)

#ifdef NDEBUG
#define TACHYON_ASSERT(cond, msg) ((void)0)
#else
#define TACHYON_ASSERT(cond, msg) TACHYON_ASSERT_ALWAYS(cond, msg)
#endif
```

- [ ] **Step 3: Aggiungere assert_tests.cpp al CMakeLists.txt dei test**

Come nel Task 2 Step 4: aggiungere `assert_tests.cpp` alla lista sorgenti di TachyonTests in `tests/unit/core/CMakeLists.txt`.

- [ ] **Step 4: Build e verificare**

```
cmake --build build-ninja --target TachyonTests
build-ninja\tests\unit\TachyonTests.exe --gtest_filter="Assert*"
```

Output atteso:
```
[  PASSED  ] 2 tests.
```

- [ ] **Step 5: Commit**

```bash
git add include/tachyon/core/assert.h tests/unit/core/assert_tests.cpp tests/unit/core/CMakeLists.txt
git commit -m "feat: add TACHYON_ASSERT and TACHYON_ASSERT_ALWAYS macros"
```

---

## Task 4: Migrare audio_decoder.cpp da fprintf a TLOG_ERROR

**Files:**
- Modify: `src/audio/audio_decoder.cpp`

- [ ] **Step 1: Aggiungere include logging.h e sostituire tutti i fprintf**

In cima al file, dopo gli include esistenti, aggiungere:
```cpp
#include "tachyon/core/logging.h"
```

Poi sostituire ogni `fprintf(stderr, "[AudioDecoder] Error: ...")` con `TLOG_ERROR(...)`. Le stringhe di formato `%s`, `%d` diventano `{}` stile spdlog/fmt.

Esempi delle 10 righe da cambiare (usare i numeri di riga del grep iniziale come riferimento, da riga 67 a 137):

```cpp
// PRIMA (riga 67):
fprintf(stderr, "[AudioDecoder] Error: FFmpeg not found during compilation. Cannot open %s\n", path.string().c_str());
// DOPO:
TLOG_ERROR("[AudioDecoder] FFmpeg not found during compilation. Cannot open {}", path.string());

// PRIMA (riga 74):
fprintf(stderr, "[AudioDecoder] Error: Could not open input file %s: %s\n", path.string().c_str(), errbuf);
// DOPO:
TLOG_ERROR("[AudioDecoder] Could not open input file {}: {}", path.string(), errbuf);

// PRIMA (riga 79):
fprintf(stderr, "[AudioDecoder] Error: Could not find stream info for %s\n", path.string().c_str());
// DOPO:
TLOG_ERROR("[AudioDecoder] Could not find stream info for {}", path.string());

// PRIMA (riga 85):
fprintf(stderr, "[AudioDecoder] Error: Could not find audio stream in %s\n", path.string().c_str());
// DOPO:
TLOG_ERROR("[AudioDecoder] Could not find audio stream in {}", path.string());

// PRIMA (riga 92):
fprintf(stderr, "[AudioDecoder] Error: Decoder not found for codec ID %d in %s\n", stream->codecpar->codec_id, path.string().c_str());
// DOPO:
TLOG_ERROR("[AudioDecoder] Decoder not found for codec ID {} in {}", stream->codecpar->codec_id, path.string());

// PRIMA (riga 98):
fprintf(stderr, "[AudioDecoder] Error: Could not allocate codec context for %s\n", path.string().c_str());
// DOPO:
TLOG_ERROR("[AudioDecoder] Could not allocate codec context for {}", path.string());

// PRIMA (riga 103):
fprintf(stderr, "[AudioDecoder] Error: Could not copy codec parameters for %s\n", path.string().c_str());
// DOPO:
TLOG_ERROR("[AudioDecoder] Could not copy codec parameters for {}", path.string());

// PRIMA (riga 108):
fprintf(stderr, "[AudioDecoder] Error: Could not open codec for %s\n", path.string().c_str());
// DOPO:
TLOG_ERROR("[AudioDecoder] Could not open codec for {}", path.string());

// PRIMA (riga 130):
fprintf(stderr, "[AudioDecoder] Error: Could not initialize resampler for %s\n", path.string().c_str());
// DOPO:
TLOG_ERROR("[AudioDecoder] Could not initialize resampler for {}", path.string());

// PRIMA (riga 137):
fprintf(stderr, "[AudioDecoder] Error: Could not allocate FFmpeg primitives for %s\n", path.string().c_str());
// DOPO:
TLOG_ERROR("[AudioDecoder] Could not allocate FFmpeg primitives for {}", path.string());
```

- [ ] **Step 2: Build per verificare**

```
cmake --build build-ninja --target TachyonCore 2>&1 | grep -E "error:|warning:" | grep "audio_decoder"
```

Output atteso: nessuna riga di errore da `audio_decoder.cpp`.

- [ ] **Step 3: Commit**

```bash
git add src/audio/audio_decoder.cpp
git commit -m "refactor: migrate audio_decoder fprintf to TLOG_ERROR"
```

---

## Task 5: Migrare audio_encoder.cpp da fprintf a TLOG_ERROR

**Files:**
- Modify: `src/audio/audio_encoder.cpp`

- [ ] **Step 1: Aggiungere include e sostituire tutti gli 8 fprintf**

In cima aggiungere:
```cpp
#include "tachyon/core/logging.h"
```

Sostituzioni (righe 80–154):

```cpp
// riga 80:
// PRIMA: fprintf(stderr, "[AudioEncoder] Error: FFmpeg not found during compilation. Cannot open %s\n", output_path.string().c_str());
TLOG_ERROR("[AudioEncoder] FFmpeg not found during compilation. Cannot open {}", output_path.string());

// riga 85:
// PRIMA: fprintf(stderr, "[AudioEncoder] Error: Could not find encoder for %s\n", config.codec.c_str());
TLOG_ERROR("[AudioEncoder] Could not find encoder for {}", config.codec);

// riga 91:
// PRIMA: fprintf(stderr, "[AudioEncoder] Error: Could not allocate format context for %s\n", output_path.string().c_str());
TLOG_ERROR("[AudioEncoder] Could not allocate format context for {}", output_path.string());

// riga 105:
// PRIMA: fprintf(stderr, "[AudioEncoder] Error: Could not open output file %s: %s\n", output_path.string().c_str(), errbuf);
TLOG_ERROR("[AudioEncoder] Could not open output file {}: {}", output_path.string(), errbuf);

// riga 118:
// PRIMA: fprintf(stderr, "[AudioEncoder] Error: Could not allocate codec context for %s\n", output_path.string().c_str());
TLOG_ERROR("[AudioEncoder] Could not allocate codec context for {}", output_path.string());

// riga 132:
// PRIMA: fprintf(stderr, "[AudioEncoder] Error: Could not open codec %s for %s\n", config.codec.c_str(), output_path.string().c_str());
TLOG_ERROR("[AudioEncoder] Could not open codec {} for {}", config.codec, output_path.string());

// riga 139:
// PRIMA: fprintf(stderr, "[AudioEncoder] Error: Could not create new stream for %s\n", output_path.string().c_str());
TLOG_ERROR("[AudioEncoder] Could not create new stream for {}", output_path.string());

// riga 154:
// PRIMA: fprintf(stderr, "[AudioEncoder] Error: Could not write header for %s\n", output_path.string().c_str());
TLOG_ERROR("[AudioEncoder] Could not write header for {}", output_path.string());
```

- [ ] **Step 2: Build**

```
cmake --build build-ninja --target TachyonCore 2>&1 | grep -E "error:" | grep "audio_encoder"
```

Output atteso: nessun errore.

- [ ] **Step 3: Commit**

```bash
git add src/audio/audio_encoder.cpp
git commit -m "refactor: migrate audio_encoder fprintf to TLOG_ERROR"
```

---

## Task 6: Migrare audio_export.cpp e debug_export.cpp

**Files:**
- Modify: `src/audio/audio_export.cpp`
- Modify: `src/debug/debug_export.cpp`

- [ ] **Step 1: Sistemare audio_export.cpp (std::cout di debug)**

In `src/audio/audio_export.cpp` riga 163, sostituire il `std::cout` (debug print lasciato in produzione) con `TLOG_INFO`:

```cpp
// Aggiungere in cima:
#include "tachyon/core/logging.h"

// PRIMA (riga 163):
std::cout << "[Audio Export] Integrated LUFS: " << m_loudness_measurement.integrated_lufs 
// DOPO — stessa logica, mostra il valore:
TLOG_INFO("[AudioExport] Integrated LUFS: {:.1f}", m_loudness_measurement.integrated_lufs);
// (rimuovere il resto della riga cout che continua con << ... << '\n')
```

- [ ] **Step 2: Sistemare debug_export.cpp (tutti std::cerr)**

In `src/debug/debug_export.cpp` aggiungere in cima:
```cpp
#include "tachyon/core/logging.h"
```

Sostituire le 8 righe (righe 13–95):

```cpp
// riga 13: errore parametri invalidi
// PRIMA: std::cerr << "[DebugExport] Invalid parameters for RGBA export\n";
TLOG_ERROR("[DebugExport] Invalid parameters for RGBA export");

// riga 26: errore scrittura PNG
// PRIMA: std::cerr << "[DebugExport] Failed to write RGBA PNG: " << path << '\n';
TLOG_ERROR("[DebugExport] Failed to write RGBA PNG: {}", path.string());

// riga 30: successo — usare INFO, non WARN/ERROR
// PRIMA: std::cerr << "[DebugExport] Exported RGBA to: " << path << '\n';
TLOG_INFO("[DebugExport] Exported RGBA to: {}", path.string());

// riga 36: errore parametri float
// PRIMA: std::cerr << "[DebugExport] Invalid parameters for float buffer export\n";
TLOG_ERROR("[DebugExport] Invalid parameters for float buffer export");

// riga 58: errore scrittura float PNG
// PRIMA: std::cerr << "[DebugExport] Failed to write float buffer PNG: " << path << '\n';
TLOG_ERROR("[DebugExport] Failed to write float buffer PNG: {}", path.string());

// riga 62: successo float
// PRIMA: std::cerr << "[DebugExport] Exported float buffer '" << channel_name << "' to: " << path << '\n';
TLOG_INFO("[DebugExport] Exported float buffer '{}' to: {}", channel_name, path.string());

// riga 75: errore apertura JSON
// PRIMA: std::cerr << "[DebugExport] Failed to open JSON file: " << path << '\n';
TLOG_ERROR("[DebugExport] Failed to open JSON file: {}", path.string());

// riga 83: errore scrittura JSON
// PRIMA: std::cerr << "[DebugExport] Failed to write JSON file: " << path << '\n';
TLOG_ERROR("[DebugExport] Failed to write JSON file: {}", path.string());

// riga 87: successo JSON
// PRIMA: std::cerr << "[DebugExport] Exported JSON to: " << path << '\n';
TLOG_INFO("[DebugExport] Exported JSON to: {}", path.string());

// riga 95: info directory creata
// PRIMA: std::cerr << "[DebugExport] Created snapshot directory: " << dir.string() << '\n';
TLOG_INFO("[DebugExport] Created snapshot directory: {}", dir.string());
```

- [ ] **Step 3: Build**

```
cmake --build build-ninja --target TachyonCore 2>&1 | grep -E "error:" | grep -E "audio_export|debug_export"
```

Output atteso: nessun errore.

- [ ] **Step 4: Commit**

```bash
git add src/audio/audio_export.cpp src/debug/debug_export.cpp
git commit -m "refactor: migrate audio_export cout and debug_export cerr to TLOG_*"
```

---

## Task 7: Fix profiler thread-safety

**Files:**
- Modify: `include/tachyon/renderer3d/utilities/debug_tools.h`
- Modify: `src/renderer3d/utilities/debug_tools.cpp`
- Create: `tests/unit/renderer3d/profiler_tests.cpp`

- [ ] **Step 1: Scrivere il test thread-safety (deve fallire con il codice attuale)**

Creare `tests/unit/renderer3d/profiler_tests.cpp`:

```cpp
#include <gtest/gtest.h>
#include "tachyon/renderer3d/utilities/debug_tools.h"
#include <thread>
#include <vector>

using namespace tachyon::renderer3d;

TEST(ProfilerData, SingleThreadBasic) {
    RendererDebug::ProfilerData p;
    p.begin_section("render");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    p.end_section("render");

    ASSERT_EQ(p.sections.size(), 1u);
    EXPECT_EQ(p.sections[0].call_count, 1);
    EXPECT_GT(p.sections[0].time_ms, 0.0);
}

TEST(ProfilerData, MultiThreadDoesNotCorruptCallCount) {
    RendererDebug::ProfilerData profiler;
    constexpr int N_THREADS = 4;
    constexpr int CALLS_PER_THREAD = 50;

    std::vector<std::thread> threads;
    for (int t = 0; t < N_THREADS; ++t) {
        threads.emplace_back([&profiler]() {
            for (int i = 0; i < CALLS_PER_THREAD; ++i) {
                profiler.begin_section("work");
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                profiler.end_section("work");
            }
        });
    }
    for (auto& th : threads) th.join();

    ASSERT_EQ(profiler.sections.size(), 1u);
    EXPECT_EQ(profiler.sections[0].call_count, N_THREADS * CALLS_PER_THREAD);
    EXPECT_GT(profiler.sections[0].time_ms, 0.0);
    EXPECT_LE(profiler.sections[0].min_time_ms, profiler.sections[0].avg_time_ms);
    EXPECT_GE(profiler.sections[0].max_time_ms, profiler.sections[0].avg_time_ms);
}

TEST(ProfilerData, ResetClearsCountsButKeepsSections) {
    RendererDebug::ProfilerData p;
    p.begin_section("a");
    p.end_section("a");
    p.reset();
    EXPECT_EQ(p.sections[0].call_count, 0);
    EXPECT_EQ(p.sections[0].time_ms, 0.0);
}
```

- [ ] **Step 2: Aggiungere il test al CMakeLists dei test**

Controllare se esiste `tests/unit/renderer3d/CMakeLists.txt`:
```bash
ls tests/unit/renderer3d/
```
Se non esiste la directory, crearla e aggiungerla a `tests/unit/CMakeLists.txt`. Se esiste già, aggiungere `profiler_tests.cpp` alla lista sorgenti di TachyonTests.

- [ ] **Step 3: Verificare che il test MultiThread fallisca (race condition attiva)**

```
cmake --build build-ninja --target TachyonTests
build-ninja\tests\unit\TachyonTests.exe --gtest_filter="ProfilerData.MultiThread*"
```

Output atteso: il test `MultiThreadDoesNotCorruptCallCount` fallisce (call_count errato) O il processo crasha su data race. Se passa sempre, eseguire 5 volte con `--gtest_repeat=5`.

- [ ] **Step 4: Modificare debug_tools.h — rimuovere section_start_ e current_section_, aggiungere mutex**

In `include/tachyon/renderer3d/utilities/debug_tools.h`, modificare `ProfilerData`:

```cpp
// Aggiungere include all'inizio del file:
#include <mutex>

struct ProfilerData {
    struct Section {
        std::string name;
        double time_ms{0.0};
        int call_count{0};
        double min_time_ms{0.0};
        double max_time_ms{0.0};
        double avg_time_ms{0.0};
    };

    std::vector<Section> sections;
    std::unordered_map<std::string, std::size_t> section_indices;
    mutable std::mutex mutex_;
    // RIMOSSO: section_start_, current_section_

    void begin_section(const std::string& name);
    void end_section(const std::string& name);
    void reset();

    std::string report() const;
    std::string report_json() const;
};
```

- [ ] **Step 5: Modificare debug_tools.cpp — implementare begin/end con thread_local**

In `src/renderer3d/utilities/debug_tools.cpp`, nel namespace `tachyon::renderer3d`, aggiungere prima delle implementazioni:

```cpp
#include <mutex>

namespace {
    thread_local std::unordered_map<std::string,
        std::chrono::high_resolution_clock::time_point> tl_section_starts;
} // namespace
```

Poi sostituire `begin_section` e `end_section`:

```cpp
void RendererDebug::ProfilerData::begin_section(const std::string& name) {
    tl_section_starts[name] = std::chrono::high_resolution_clock::now();

    std::lock_guard<std::mutex> lock(mutex_);
    if (section_indices.find(name) == section_indices.end()) {
        section_indices[name] = sections.size();
        sections.push_back({name, 0.0, 0, 1e100, 0.0, 0.0});
    }
}

void RendererDebug::ProfilerData::end_section(const std::string& name) {
    auto end_time = std::chrono::high_resolution_clock::now();

    auto start_it = tl_section_starts.find(name);
    if (start_it == tl_section_starts.end()) return;
    double elapsed = std::chrono::duration<double, std::milli>(
        end_time - start_it->second).count();

    std::lock_guard<std::mutex> lock(mutex_);
    auto idx_it = section_indices.find(name);
    if (idx_it == section_indices.end()) return;

    auto& section = sections[idx_it->second];
    section.call_count++;
    section.time_ms += elapsed;
    section.min_time_ms = std::min(section.min_time_ms, elapsed);
    section.max_time_ms = std::max(section.max_time_ms, elapsed);
    section.avg_time_ms = section.time_ms / static_cast<double>(section.call_count);
}
```

Anche `reset()` deve essere thread-safe:
```cpp
void RendererDebug::ProfilerData::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& section : sections) {
        section.time_ms = 0.0;
        section.call_count = 0;
        section.min_time_ms = 1e100;
        section.max_time_ms = 0.0;
        section.avg_time_ms = 0.0;
    }
}
```

- [ ] **Step 6: Build e verificare che tutti i test profiler passino**

```
cmake --build build-ninja --target TachyonTests
build-ninja\tests\unit\TachyonTests.exe --gtest_filter="ProfilerData*" --gtest_repeat=3
```

Output atteso: `[  PASSED  ] 3 tests.` su tutte le ripetizioni.

- [ ] **Step 7: Commit**

```bash
git add include/tachyon/renderer3d/utilities/debug_tools.h src/renderer3d/utilities/debug_tools.cpp tests/unit/renderer3d/profiler_tests.cpp
git commit -m "fix: make ProfilerData thread-safe with mutex + thread_local timestamps"
```

---

## Task 8: Tracy integration (opzionale, off by default)

**Files:**
- Modify: `CMakeLists.txt`
- Create: `include/tachyon/core/profiling.h`
- Modify: `src/runtime/execution/render_session.cpp` (aggiungere zone markers)

- [ ] **Step 1: Aggiungere Tracy come opzione CMake**

In `CMakeLists.txt`, dopo il blocco spdlog (Task 1), aggiungere:

```cmake
# Tracy Profiler — opzionale, off by default
option(TACHYON_ENABLE_TRACY "Enable Tracy profiler integration" OFF)
if(TACHYON_ENABLE_TRACY)
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG        v0.11.1
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
    set(TRACY_ON_DEMAND ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(tracy)
    message(STATUS "[Tachyon] Tracy profiler enabled")
endif()
```

In `src/CMakeLists.txt`, dopo il blocco `target_link_libraries(TachyonCore ...)`:

```cmake
if(TACHYON_ENABLE_TRACY)
    target_link_libraries(TachyonCore PUBLIC TracyClient)
    target_compile_definitions(TachyonCore PUBLIC TACHYON_TRACY_ENABLED)
endif()
```

- [ ] **Step 2: Creare include/tachyon/core/profiling.h**

```cpp
#pragma once

#ifdef TACHYON_TRACY_ENABLED
    #include <tracy/Tracy.hpp>
    #define TACHYON_ZONE()              ZoneScoped
    #define TACHYON_ZONE_N(name)        ZoneScopedN(name)
    #define TACHYON_FRAME_MARK()        FrameMark
    #define TACHYON_FRAME_MARK_N(name)  FrameMarkNamed(name)
    #define TACHYON_MSG(msg)            TracyMessage(msg, sizeof(msg) - 1)
#else
    #define TACHYON_ZONE()
    #define TACHYON_ZONE_N(name)
    #define TACHYON_FRAME_MARK()
    #define TACHYON_FRAME_MARK_N(name)
    #define TACHYON_MSG(msg)
#endif
```

- [ ] **Step 3: Aggiungere zone markers in render_session.cpp**

In `src/runtime/execution/render_session.cpp`, aggiungere in cima:
```cpp
#include "tachyon/core/profiling.h"
```

All'inizio del metodo `render_frames_parallel` (o `execute`), aggiungere:
```cpp
TACHYON_ZONE_N("RenderSession::render_frames_parallel");
```

All'interno del loop che processa ogni frame, aggiungere:
```cpp
TACHYON_FRAME_MARK_N("frame");
```

- [ ] **Step 4: Verificare build senza Tracy (default)**

```
cmake --preset relwithdebinfo
cmake --build build-ninja --target TachyonCore
```

Output atteso: build OK, nessuna menzione di Tracy negli output.

- [ ] **Step 5: Verificare build CON Tracy**

```
cmake --preset relwithdebinfo -DTACHYON_ENABLE_TRACY=ON
cmake --build build-ninja --target TachyonCore
```

Output atteso: `[Tachyon] Tracy profiler enabled` durante configure, build OK.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt src/CMakeLists.txt include/tachyon/core/profiling.h src/runtime/execution/render_session.cpp
git commit -m "feat: add Tracy profiler integration (off by default, -DTACHYON_ENABLE_TRACY=ON)"
```

---

## Checklist finale (self-review)

- [x] **spdlog** — FetchContent + link + macro TLOG_* ✓
- [x] **TACHYON_ASSERT** — macro con file/line/message, disabilitabile in release ✓  
- [x] **Profiler thread-safe** — `thread_local` starts + `mutex_` per sezioni ✓
- [x] **Tracy** — opzionale, off by default, macro no-op quando disabilitato ✓
- [x] **Migrazione fprintf** — audio_decoder (10 righe), audio_encoder (8 righe) ✓
- [x] **Migrazione cerr/cout** — audio_export (1 riga), debug_export (10 righe) ✓
- [x] **cli.cpp non toccata** — usa cout/cerr intenzionalmente per output utente ✓
- [x] **Test per ogni componente** — logging, assert, profiler thread-safety ✓
- [x] **Ogni task compila e testa prima del commit** ✓
