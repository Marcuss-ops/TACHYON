#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace tachyon::text {

struct ScriptInfo {
    const char* script;
    const char* language;
};

class ScriptDetector {
public:
    static ScriptInfo detect_script(std::uint32_t codepoint);
    
    // Detects predominant script/language for a run
    static ScriptInfo detect_run_info(const std::uint32_t* codepoints, std::size_t count);
};

} // namespace tachyon::text
