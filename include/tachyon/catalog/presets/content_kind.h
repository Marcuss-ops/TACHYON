#pragma once

namespace tachyon::content {

/**
 * @brief Identifies the type of content managed by the ContentCatalog.
 */
enum class ContentKind {
    Background,
    TextLayer,
    TextAnimator,
    Transition,
    Sfx
};

} // namespace tachyon::content
