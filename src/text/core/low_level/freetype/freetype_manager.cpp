#include "tachyon/text/core/low_level/freetype/freetype_manager.h"

namespace tachyon::renderer2d::text {
namespace {

struct FTManager {
    FT_Library library{nullptr};
    FTManager() { FT_Init_FreeType(&library); }
    ~FTManager() { if (library) FT_Done_FreeType(library); }
};

} // namespace

FT_Library get_ft_library() {
    static FTManager manager;
    return manager.library;
}

} // namespace tachyon::renderer2d::text
