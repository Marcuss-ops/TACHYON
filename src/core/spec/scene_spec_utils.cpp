#include "tachyon/core/spec/schema/objects/scene_spec.h"

#include <optional>
#include <set>
#include <string>

#include "tachyon/base/logging.h"
#include "tachyon/math/vector2.h"
#include "tachyon/math/vector3.h"
#include "tachyon/renderer/renderer2d.h"

namespace tachyon {

void parse_optional_vector3_property(
    const json& object, const char* key,
    AnimatedVector3Spec& property,
    const std::string& path,
    DiagnosticBag& diagnostics) {
  if (!object.contains(key) || object.at(key).is_null()) return;
  const auto& value = object.at(key);
  math::Vector3 v;
  if (parse_vector3_value(value, v)) {
    property.set_value(v);
    return;
  }
  if (value.is_object()) {
    if (value.contains("value") && parse_vector3_value(value.at("value"), v)) {
      property.set_value(v);
    }
    if (value.contains("keyframes") &&
        value.at("keyframes").is_array()) {
      const auto& keyframes = value.at("keyframes");
      for (std::size_t i = 0; i < keyframes.size(); ++i) {
        AnimatedVector3Spec::Keyframe k;
        if (parse_vector3_keyframe(keyframes[i], k,
                                    make_path(path, key) +
                                        ".keyframes[" +
                                        std::to_string(i) + "]",
                                    diagnostics)) {
          property.add_keyframe(k);
        }
      }
    }
    if (value.contains("expression") &&
        value.at("expression").is_string()) {
      property.set_expression(value.at("expression").get<std::string>());
    }
    return;
  }
  diagnostics.add_error("scene.layer.property_invalid",
                        std::string(key) +
                            " must contain a vector value, keyframes, or "
                            "expression when provided as an object",
                        make_path(path, key));
}

}  // namespace tachyon