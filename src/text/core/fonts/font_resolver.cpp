#include "tachyon/text/fonts/font_resolver.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace tachyon::text {

void FontResolver::clear() {
    m_faces.clear();
    m_fallback_chains.clear();
    m_instances.clear();
}

void FontResolver::register_face(const FontFace& face, const std::string& family,
                                 std::uint32_t weight, FontStyle style) {
    FaceKey key{family, weight, style};
    m_faces[key] = &face;
}

void FontResolver::set_fallback_chain(const std::string& family,
                                      const std::vector<std::string>& fallbacks) {
    m_fallback_chains[family] = fallbacks;
}

ResolvedFont FontResolver::resolve(const FontRequest& request) const {
    ResolvedFont result;

    const FontFace* face = nullptr;

    auto try_weight = [&](std::uint32_t target_weight) -> const FontFace* {
        FaceKey key{request.family, target_weight, request.style};
        auto it = m_faces.find(key);
        if (it != m_faces.end()) {
            return it->second;
        }
        return nullptr;
    };

    face = try_weight(request.weight);

    if (!face && request.weight != 400) {
        face = try_weight(400);
    }
    if (!face && request.weight != 700) {
        face = try_weight(700);
    }
    if (!face && request.weight != 500) {
        face = try_weight(500);
    }

    if (!face) {
        FaceKey key{request.family, 400, FontStyle::Normal};
        auto it = m_faces.find(key);
        if (it != m_faces.end()) {
            face = it->second;
        }
    }

    if (!face) {
        auto fallback_it = m_fallback_chains.find(request.family);
        if (fallback_it != m_fallback_chains.end()) {
            for (const auto& fallback_family : fallback_it->second) {
                FontRequest fallback_req = request;
                fallback_req.family = fallback_family;
                auto fallback_result = resolve(fallback_req);
                if (fallback_result.face) {
                    result.is_fallback = true;
                    face = fallback_result.face;
                    break;
                }
            }
        }
    }

    result.face = face;

    if (face) {
        result.instance = get_or_create_instance(*face, request.pixel_size,
                                                 request.hinting, request.render_mode);
    }

    return result;
}

std::vector<const FontFace*> FontResolver::get_faces_for_family(const std::string& family) const {
    std::vector<const FontFace*> result;
    for (const auto& [key, face] : m_faces) {
        if (key.family == family) {
            result.push_back(face);
        }
    }
    return result;
}

const FontInstance* FontResolver::get_or_create_instance(const FontFace& face,
                                                        std::uint32_t pixel_size,
                                                        HintingMode hinting,
                                                        RenderMode render_mode) {
    FontInstanceKey key{
        face.id(),
        pixel_size,
        hinting,
        render_mode
    };

    auto it = m_instances.find(key);
    if (it != m_instances.end()) {
        return it->second.get();
    }

    auto instance = std::make_unique<FontInstance>();
    if (!instance->init(face, pixel_size, hinting, render_mode)) {
        return nullptr;
    }

    FontInstance* ptr = instance.get();
    m_instances[key] = std::move(instance);
    return ptr;
}

} // namespace tachyon::text
