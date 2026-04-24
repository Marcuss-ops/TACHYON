#include "tachyon/text/i18n/bidi_engine.h"
#include <cstring>

namespace tachyon::text {

namespace {

// UAX#9 character type classification
CharacterDirection get_bidi_type(std::uint32_t cp) {
    // Strong LTR
    if ((cp >= 0x0041 && cp <= 0x005A) || (cp >= 0x0061 && cp <= 0x007A) ||
        (cp >= 0x00C0 && cp <= 0x02AF) || (cp >= 0x0370 && cp <= 0x052F) ||
        (cp >= 0x4E00 && cp <= 0x9FFF) || cp == 0x00AA || cp == 0x00BA || cp == 0x00C0 ||
        (cp >= 0x00C1 && cp <= 0x00C5) || cp == 0x00C6 || cp == 0x00C7 || cp == 0x00C8 ||
        (cp >= 0x00C9 && cp <= 0x00CF) || (cp >= 0x00D1 && cp <= 0x00D6) || cp == 0x00D8 ||
        (cp >= 0x00D9 && cp <= 0x00DC) || cp == 0x00DD || (cp >= 0x00E0 && cp <= 0x00F6) ||
        cp == 0x00F8 || (cp >= 0x00F9 && cp <= 0x00FF)) {
        return CharacterDirection::LTR;
    }
    
    // Strong RTL (Arabic, Hebrew, etc.)
    if ((cp >= 0x0590 && cp <= 0x08FF) || (cp >= 0xFB50 && cp <= 0xFDFF) ||
        (cp >= 0xFE70 && cp <= 0xFEFF) || (cp >= 0x10800 && cp <= 0x10FFF) ||
        (cp >= 0x1C90 && cp <= 0x1CBF)) {
        return CharacterDirection::RTL;
    }
    
    // Arabic Letter (AL)
    if (cp >= 0x0600 && cp <= 0x06FF) {
        return CharacterDirection::AL;
    }
    
    // European Number (EN)
    if (cp >= 0x0030 && cp <= 0x0039) {
        return CharacterDirection::EN;
    }
    
    // Arabic Number (AN)
    if ((cp >= 0x0660 && cp <= 0x0669) || (cp >= 0x06F0 && cp <= 0x06F9)) {
        return CharacterDirection::AN;
    }
    
    // Separators
    if (cp == 0x002B || cp == 0x002D) return CharacterDirection::ES;
    if (cp == 0x002C || cp == 0x002E || cp == 0x003A) return CharacterDirection::CS;
    
    // Non-Spacing Mark
    if (cp >= 0x0300 && cp <= 0x036F) return CharacterDirection::NSM;
    
    // Paragraph/Segment Separator
    if (cp == 0x000A || cp == 0x000D) return CharacterDirection::B;
    if (cp == 0x0009 || cp == 0x001F || cp == 0x0020) return CharacterDirection::S;
    
    // White Space
    if (cp == 0x0020 || cp == 0x00A0 || cp == 0x2000 || cp == 0x2001 || cp == 0x2002 ||
        cp == 0x2003 || cp == 0x2004 || cp == 0x2005 || cp == 0x2006 || cp == 0x2007 ||
        cp == 0x2008 || cp == 0x2009 || cp == 0x200A || cp == 0x202F || cp == 0x205F ||
        cp == 0x3000) {
        return CharacterDirection::WS;
    }
    
    // Explicit directional controls
    if (cp == 0x202A) return CharacterDirection::LRE;
    if (cp == 0x202B) return CharacterDirection::RLE;
    if (cp == 0x202D) return CharacterDirection::LRO;
    if (cp == 0x202E) return CharacterDirection::RLO;
    if (cp == 0x202C) return CharacterDirection::PDF;
    
    return CharacterDirection::ON;
}

// UAX#9 N0-N2: Resolve neutral types
CharacterDirection resolve_neutral(
    const std::vector<CharacterDirection>& types,
    std::size_t index,
    CharacterDirection base_direction) {
    
    auto is_strong = [](CharacterDirection d) {
        return d == CharacterDirection::LTR || d == CharacterDirection::RTL || d == CharacterDirection::AL;
    };
    
    // Find surrounding strong types
    CharacterDirection left_strong = base_direction;
    CharacterDirection right_strong = base_direction;
    
    // Search left
    for (std::size_t i = index; i > 0; --i) {
        if (is_strong(types[i - 1])) {
            left_strong = types[i - 1];
            break;
        }
    }
    
    // Search right
    for (std::size_t i = index + 1; i < types.size(); ++i) {
        if (is_strong(types[i])) {
            right_strong = types[i];
            break;
        }
    }
    
    // N1: Neutral between same direction -> use that direction
    if (is_strong(left_strong) && is_strong(right_strong) && left_strong == right_strong) {
        return (left_strong == CharacterDirection::RTL || left_strong == CharacterDirection::AL) 
               ? CharacterDirection::RTL : CharacterDirection::LTR;
    }
    
    // N2: Otherwise use base direction
    return base_direction;
}

// W1-W7: Resolve weak types (simplified)
void resolve_weak_types(std::vector<CharacterDirection>& types) {
    // W1: NSM following a strong type inherits that type
    for (std::size_t i = 1; i < types.size(); ++i) {
        if (types[i] == CharacterDirection::NSM) {
            types[i] = types[i - 1];
        }
    }
    
    // W2: EN following AL/R becomes AN
    for (std::size_t i = 1; i < types.size(); ++i) {
        if (types[i] == CharacterDirection::EN) {
            for (std::size_t j = i; j > 0; --j) {
                if (types[j - 1] == CharacterDirection::AL || types[j - 1] == CharacterDirection::RTL) {
                    types[i] = CharacterDirection::AN;
                    break;
                }
                if (types[j - 1] == CharacterDirection::LTR) break;
            }
        }
    }
}

} // namespace

CharacterDirection BidiEngine::get_direction(std::uint32_t codepoint) {
    return get_bidi_type(codepoint);
}

std::vector<BidiRun> BidiEngine::analyze(const std::vector<std::uint32_t>& codepoints) {
    if (codepoints.empty()) return {};
    
    std::vector<BidiRun> all_runs;
    
    // 1. Paragraph Segmentation
    std::size_t paragraph_start = 0;
    for (std::size_t i = 0; i <= codepoints.size(); ++i) {
        bool is_para_break = (i == codepoints.size()) || (codepoints[i] == '\n' || codepoints[i] == '\r');
        if (is_para_break) {
            const std::size_t length = i - paragraph_start;
            if (length > 0 || i < codepoints.size()) {
                auto para_runs = analyze_paragraph(codepoints, paragraph_start, length);
                all_runs.insert(all_runs.end(), para_runs.begin(), para_runs.end());
            }
            paragraph_start = i + 1;
        }
    }
    
    return all_runs;
}

std::vector<BidiRun> BidiEngine::analyze_paragraph(const std::vector<std::uint32_t>& codepoints, std::size_t start, std::size_t length) {
    if (length == 0) {
        return {{start, 0, CharacterDirection::LTR, 0}};
    }
    
    std::vector<BidiRun> runs;
    std::vector<CharacterDirection> types(length);
    
    // 2. Base Direction Detection (P2-P3)
    CharacterDirection base_direction = CharacterDirection::LTR;
    for (std::size_t i = 0; i < length; ++i) {
        types[i] = get_bidi_type(codepoints[start + i]);
        if (types[i] == CharacterDirection::LTR || types[i] == CharacterDirection::RTL || types[i] == CharacterDirection::AL) {
            base_direction = (types[i] == CharacterDirection::LTR) ? CharacterDirection::LTR : CharacterDirection::RTL;
            break;
        }
    }
    
    // W1-W7: Resolve weak types
    resolve_weak_types(types);
    
    // N0-N2: Resolve neutral types
    for (std::size_t i = 0; i < length; ++i) {
        if (types[i] == CharacterDirection::ON || types[i] == CharacterDirection::WS ||
            types[i] == CharacterDirection::CS || types[i] == CharacterDirection::ES) {
            types[i] = resolve_neutral(types, i, base_direction);
        }
    }
    
    // I1-I2: Implicit level assignment (simplified)
    for (auto& t : types) {
        if (t == CharacterDirection::AL) t = CharacterDirection::RTL;
    }
    
    // Build Runs
    CharacterDirection current_run_dir = types[0];
    std::size_t run_start = 0;
    for (std::size_t i = 1; i < length; ++i) {
        if (types[i] != current_run_dir) {
            int level = (current_run_dir == CharacterDirection::RTL) ? 1 : 0;
            runs.push_back({start + run_start, i - run_start, current_run_dir, level});
            run_start = i;
            current_run_dir = types[i];
        }
    }
    
    int level = (current_run_dir == CharacterDirection::RTL) ? 1 : 0;
    runs.push_back({start + run_start, length - run_start, current_run_dir, level});
    
    return runs;
}

} // namespace tachyon::text
