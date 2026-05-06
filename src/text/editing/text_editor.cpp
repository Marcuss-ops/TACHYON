#include "tachyon/text/animation/text_editor.h"
#include "tachyon/text/core/low_level/utf8/utf8_decoder.h"
#include "tachyon/text/layout/cluster_iterator.h"
#include <cctype>
#include <algorithm>

namespace tachyon::text {

// --- TextDocument ---

void TextDocument::set_utf8(const std::string& utf8_text) {
    m_codepoints = renderer2d::text::utf8::decode_utf8(utf8_text);
    rebuild_clusters();
}

std::string TextDocument::encode_utf8(const std::vector<std::uint32_t>& codepoints) {
    std::string result;
    for (std::uint32_t cp : codepoints) {
        if (cp <= 0x7F) {
            result += static_cast<char>(cp);
        } else if (cp <= 0x7FF) {
            result += static_cast<char>(0xC0 | ((cp >> 6) & 0x1F));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp <= 0xFFFF) {
            result += static_cast<char>(0xE0 | ((cp >> 12) & 0x0F));
            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp <= 0x10FFFF) {
            result += static_cast<char>(0xF0 | ((cp >> 18) & 0x07));
            result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
    return result;
}

std::string TextDocument::get_utf8() const {
    return encode_utf8(m_codepoints);
}

void TextDocument::rebuild_clusters() {
    m_clusters = ClusterIterator::segment(m_codepoints);
}

void TextDocument::delete_clusters(std::size_t start_cluster, std::size_t end_cluster) {
    if (start_cluster >= m_clusters.size() || start_cluster >= end_cluster) return;
    
    std::size_t cp_start = m_clusters[start_cluster].start_index;
    std::size_t cp_end = (end_cluster < m_clusters.size()) ? m_clusters[end_cluster].start_index : m_codepoints.size();
    
    m_codepoints.erase(m_codepoints.begin() + cp_start, m_codepoints.begin() + cp_end);
    rebuild_clusters();
}

void TextDocument::insert_at_cluster(std::size_t cluster_index, const std::vector<std::uint32_t>& codepoints) {
    if (codepoints.empty()) return;
    
    std::size_t cp_index = m_codepoints.size();
    if (cluster_index < m_clusters.size()) {
        cp_index = m_clusters[cluster_index].start_index;
    }
    
    m_codepoints.insert(m_codepoints.begin() + cp_index, codepoints.begin(), codepoints.end());
    rebuild_clusters();
}

// --- TextEditor ---

void TextEditor::set_text(const std::string& utf8_text) {
    m_document.set_utf8(utf8_text);
    m_selection.anchor = m_document.cluster_count();
    m_selection.focus = m_document.cluster_count();
}

std::string TextEditor::get_display_text() const {
    if (!m_ime_state.active || m_ime_state.composition_text.empty()) {
        return get_text();
    }

    std::vector<std::uint32_t> display_cps = m_document.get_codepoints();
    auto ime_cps = renderer2d::text::utf8::decode_utf8(m_ime_state.composition_text);
    
    std::size_t cp_insert_pos = m_document.get_codepoints().size();
    if (m_selection.focus < m_document.cluster_count()) {
        cp_insert_pos = m_document.get_clusters()[m_selection.focus].start_index;
    }
    
    display_cps.insert(display_cps.begin() + cp_insert_pos, ime_cps.begin(), ime_cps.end());
    return TextDocument::encode_utf8(display_cps);
}

void TextEditor::set_selection(std::size_t anchor_cluster, std::size_t focus_cluster) {
    m_selection.anchor = std::min(anchor_cluster, m_document.cluster_count());
    m_selection.focus = std::min(focus_cluster, m_document.cluster_count());
}

void TextEditor::clear_selection() {
    m_selection.anchor = m_selection.focus;
}

void TextEditor::insert_text(const std::string& utf8_text) {
    delete_selection();
    auto new_codepoints = renderer2d::text::utf8::decode_utf8(utf8_text);
    if (new_codepoints.empty()) return;

    std::size_t original_cluster_count = m_document.cluster_count();
    m_document.insert_at_cluster(m_selection.focus, new_codepoints);
    
    std::size_t inserted_clusters = m_document.cluster_count() - original_cluster_count;
    m_selection.focus += inserted_clusters;
    m_selection.anchor = m_selection.focus;
}

void TextEditor::delete_selection() {
    if (m_selection.is_empty()) return;
    
    std::size_t min_idx = m_selection.min();
    std::size_t max_idx = m_selection.max();
    
    m_document.delete_clusters(min_idx, max_idx);
    m_selection.anchor = min_idx;
    m_selection.focus = min_idx;
}

void TextEditor::delete_backward() {
    if (!m_selection.is_empty()) {
        delete_selection();
        return;
    }
    if (m_selection.focus > 0) {
        m_selection.focus--;
        m_document.delete_clusters(m_selection.focus, m_selection.focus + 1);
        m_selection.anchor = m_selection.focus;
    }
}

void TextEditor::delete_forward() {
    if (!m_selection.is_empty()) {
        delete_selection();
        return;
    }
    if (m_selection.focus < m_document.cluster_count()) {
        m_document.delete_clusters(m_selection.focus, m_selection.focus + 1);
    }
}

void TextEditor::set_composition(const std::string& text, std::size_t cursor_cluster) {
    m_ime_state.active = true;
    m_ime_state.composition_text = text;
    m_ime_state.cursor_cluster_index = cursor_cluster;
}

void TextEditor::commit_composition() {
    if (m_ime_state.active && !m_ime_state.composition_text.empty()) {
        insert_text(m_ime_state.composition_text);
    }
    clear_composition();
}

void TextEditor::clear_composition() {
    m_ime_state.active = false;
    m_ime_state.composition_text.clear();
    m_ime_state.cursor_cluster_index = 0;
}

void TextEditor::update_layout(const TextLayoutResult& layout) {
    m_last_layout = layout;
}

void TextEditor::hit_test_and_set_selection(int x, int y, bool shift_pressed) {
    auto res = m_last_layout.hit_test(x, y);
    if (!res.inside_text) return;

    if (shift_pressed) {
        m_selection.focus = res.cluster_index;
    } else {
        m_selection.anchor = res.cluster_index;
        m_selection.focus = res.cluster_index;
    }
}

void TextEditor::move_caret_left(bool shift_pressed) {
    if (m_selection.focus > 0) {
        m_selection.focus--;
        if (!shift_pressed) {
            m_selection.anchor = m_selection.focus;
        }
    } else if (!shift_pressed) {
        m_selection.anchor = 0;
    }
}

void TextEditor::move_caret_right(bool shift_pressed) {
    if (m_selection.focus < m_document.cluster_count()) {
        m_selection.focus++;
        if (!shift_pressed) {
            m_selection.anchor = m_selection.focus;
        }
    } else if (!shift_pressed) {
        m_selection.anchor = m_document.cluster_count();
    }
}

static bool is_word_char(std::uint32_t cp) {
    if (cp <= 127) {
        return (cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z') || (cp >= '0' && cp <= '9') || cp == '_';
    }
    // Heuristic word detection for non-Latin scripts.
    // This groups common CJK, Arabic, and Hebrew letter runs.
    return cp >= 0x00A0 && cp != 0x3000 && cp != 0x200B && cp != 0x00AD;
}

static bool is_unicode_space(std::uint32_t cp) {
    if (cp <= 0x7F) {
        return std::isspace(static_cast<unsigned char>(cp)) != 0;
    }
    return cp == 0x00A0 ||
           cp == 0x1680 ||
           (cp >= 0x2000 && cp <= 0x200A) ||
           cp == 0x202F ||
           cp == 0x205F ||
           cp == 0x3000;
}

void TextEditor::move_caret_word_left(bool shift_pressed) {
    if (m_selection.focus == 0) return;

    std::size_t current = m_selection.focus;
    const auto& codepoints = m_document.get_codepoints();
    const auto& clusters = m_document.get_clusters();

    // Skip trailing whitespace
    while (current > 0) {
        std::uint32_t cp = codepoints[clusters[current - 1].start_index];
        if (!is_unicode_space(cp)) break;
        current--;
    }

    // Skip the current word run.
    if (current > 0) {
        bool in_word = is_word_char(codepoints[clusters[current - 1].start_index]);
        while (current > 0) {
            std::uint32_t cp = codepoints[clusters[current - 1].start_index];
            if (is_word_char(cp) != in_word || is_unicode_space(cp)) break;
            current--;
        }
    }

    m_selection.focus = current;
    if (!shift_pressed) m_selection.anchor = m_selection.focus;
}

void TextEditor::move_caret_word_right(bool shift_pressed) {
    std::size_t count = m_document.cluster_count();
    if (m_selection.focus >= count) return;

    std::size_t current = m_selection.focus;
    const auto& codepoints = m_document.get_codepoints();
    const auto& clusters = m_document.get_clusters();

    // Skip leading whitespace
    while (current < count) {
        std::uint32_t cp = codepoints[clusters[current].start_index];
        if (!is_unicode_space(cp)) break;
        current++;
    }

    // Skip the current word run.
    if (current < count) {
        bool in_word = is_word_char(codepoints[clusters[current].start_index]);
        while (current < count) {
            std::uint32_t cp = codepoints[clusters[current].start_index];
            if (is_word_char(cp) != in_word || is_unicode_space(cp)) break;
            current++;
        }
    }

    m_selection.focus = current;
    if (!shift_pressed) m_selection.anchor = m_selection.focus;
}

void TextEditor::select_word_at(std::size_t cluster_index) {
    std::size_t count = m_document.cluster_count();
    if (cluster_index >= count) {
        m_selection.anchor = count;
        m_selection.focus = count;
        return;
    }

    const auto& codepoints = m_document.get_codepoints();
    const auto& clusters = m_document.get_clusters();
    
    std::uint32_t pivot_cp = codepoints[clusters[cluster_index].start_index];
    bool in_word = is_word_char(pivot_cp);
    
    std::size_t start = cluster_index;
    while (start > 0) {
        if (is_word_char(codepoints[clusters[start - 1].start_index]) != in_word) break;
        start--;
    }
    
    std::size_t end = cluster_index;
    while (end < count) {
        if (is_word_char(codepoints[clusters[end].start_index]) != in_word) break;
        end++;
    }
    
    m_selection.anchor = start;
    m_selection.focus = end;
}

} // namespace tachyon::text
