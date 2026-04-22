#include "tachyon/text/i18n/script_detector.h"

namespace tachyon::text {

/**
 * @brief Simplified Unicode script detection.
 * 
 * Note: This implements a fallback script detection mechanism to assign
 * basic HarfBuzz scripts (Latin, Arabic, Hebrew, CJK, etc.) based on
 * codepoint ranges. It is NOT a comprehensive Unicode script analyzer.
 */
ScriptInfo ScriptDetector::detect_script(std::uint32_t cp) {
    if (cp >= 0x0590 && cp <= 0x05FF) return {"hebr", "he"};
    if (cp >= 0x0600 && cp <= 0x06FF) return {"arab", "ar"};
    if (cp >= 0x0700 && cp <= 0x074F) return {"syrc", "syr"};
    if (cp >= 0x08A0 && cp <= 0x08FF) return {"arab", "ar"};
    if (cp >= 0x0900 && cp <= 0x097F) return {"deva", "hi"};
    if (cp >= 0x0E00 && cp <= 0x0E7F) return {"thai", "th"};
    if (cp >= 0x1100 && cp <= 0x11FF) return {"hang", "ko"};
    if (cp >= 0x3040 && cp <= 0x309F) return {"hira", "ja"};
    if (cp >= 0x30A0 && cp <= 0x30FF) return {"kana", "ja"};
    if (cp >= 0x4E00 && cp <= 0x9FFF) return {"hani", "zh"};
    if (cp >= 0xAC00 && cp <= 0xD7AF) return {"hang", "ko"};
    
    // Default to Latin
    return {"latn", "en"};
}

ScriptInfo ScriptDetector::detect_run_info(const std::uint32_t* codepoints, std::size_t count) {
    if (count == 0) return {"latn", "en"};
    
    // For simplicity, take the first strong script found
    for (std::size_t i = 0; i < count; ++i) {
        ScriptInfo info = detect_script(codepoints[i]);
        if (std::string(info.script) != "latn") {
            return info;
        }
    }
    
    return {"latn", "en"};
}

} // namespace tachyon::text
