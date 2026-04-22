#pragma once

#include "tachyon/text/layout/layout.h"
#include "tachyon/text/layout/cluster_iterator.h"
#include <string>
#include <vector>

namespace tachyon::text {

struct SelectionModel {
    std::size_t anchor{0}; // Cluster index
    std::size_t focus{0};  // Cluster index

    bool is_empty() const { return anchor == focus; }
    std::size_t min() const { return anchor < focus ? anchor : focus; }
    std::size_t max() const { return anchor > focus ? anchor : focus; }
};

struct InputMethodState {
    std::string composition_text;
    std::size_t cursor_cluster_index{0};
    bool active{false};
};

class TextDocument {
public:
    TextDocument() = default;
    void set_utf8(const std::string& utf8_text);
    std::string get_utf8() const;
    static std::string encode_utf8(const std::vector<std::uint32_t>& codepoints);
    
    const std::vector<std::uint32_t>& get_codepoints() const { return m_codepoints; }
    const std::vector<GraphemeCluster>& get_clusters() const { return m_clusters; }
    
    std::size_t cluster_count() const { return m_clusters.size(); }
    
    // Deletes clusters in range [start_cluster, end_cluster)
    void delete_clusters(std::size_t start_cluster, std::size_t end_cluster);
    
    // Inserts codepoints at the given cluster boundary
    void insert_at_cluster(std::size_t cluster_index, const std::vector<std::uint32_t>& codepoints);

private:
    void rebuild_clusters();

    std::vector<std::uint32_t> m_codepoints;
    std::vector<GraphemeCluster> m_clusters;
};

class TextEditor {
public:
    TextEditor() = default;

    void set_text(const std::string& utf8_text);
    std::string get_text() const { return m_document.get_utf8(); }
    std::string get_display_text() const;
    const TextDocument& get_document() const { return m_document; }

    // Selection operates on grapheme cluster boundaries
    void set_selection(std::size_t anchor_cluster, std::size_t focus_cluster);
    void clear_selection();
    SelectionModel get_selection() const { return m_selection; }

    // Editing Operations
    void insert_text(const std::string& utf8_text);
    void delete_selection();
    void delete_backward();
    void delete_forward();

    // IME Composition
    void set_composition(const std::string& text, std::size_t cursor_cluster);
    void commit_composition();
    void clear_composition();
    InputMethodState get_ime_state() const { return m_ime_state; }

    // Layout Integration
    void update_layout(const TextLayoutResult& layout);
    const TextLayoutResult& get_current_layout() const { return m_last_layout; }

    // Hit Testing and Interaction
    void hit_test_and_set_selection(int x, int y, bool shift_pressed);
    void move_caret_left(bool shift_pressed);
    void move_caret_right(bool shift_pressed);
    void move_caret_word_left(bool shift_pressed);
    void move_caret_word_right(bool shift_pressed);
    void select_word_at(std::size_t cluster_index);

private:
    TextDocument m_document;
    SelectionModel m_selection;
    InputMethodState m_ime_state;
    TextLayoutResult m_last_layout;
};

} // namespace tachyon::text
