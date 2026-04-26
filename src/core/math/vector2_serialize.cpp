#include "tachyon/core/math/vector2.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace tachyon {
namespace math {

void to_json(json& j, const Vector2& v) {
    j = json{{"x", v.x}, {"y", v.y}};
}

void from_json(const json& j, Vector2& v) {
    if (j.contains("x") && j.at("x").is_number()) v.x = j.at("x").get<float>();
    if (j.contains("y") && j.at("y").is_number()) v.y = j.at("y").get<float>();
}

} // namespace math
} // namespace tachyon
