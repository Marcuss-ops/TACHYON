#pragma once

#include "tachyon/core/spec/schema/properties/property_spec.h"
#include <nlohmann/json.hpp>

namespace tachyon {

// WiggleSpec serialization (used by multiple property types)
void to_json(nlohmann::json& j, const WiggleSpec& w);
void from_json(const nlohmann::json& j, WiggleSpec& w);

// ScalarKeyframeSpec serialization
void to_json(nlohmann::json& j, const ScalarKeyframeSpec& k);
void from_json(const nlohmann::json& j, ScalarKeyframeSpec& k);

// Vector2KeyframeSpec serialization  
void to_json(nlohmann::json& j, const Vector2KeyframeSpec& k);
void from_json(const nlohmann::json& j, Vector2KeyframeSpec& k);

// Vector3KeyframeSpec::Keyframe serialization
void to_json(nlohmann::json& j, const AnimatedVector3Spec::Keyframe& k);
void from_json(const nlohmann::json& j, AnimatedVector3Spec::Keyframe& k);

// ColorKeyframeSpec serialization
void to_json(nlohmann::json& j, const ColorKeyframeSpec& k);
void from_json(const nlohmann::json& j, ColorKeyframeSpec& k);

// MaskPathKeyframeSpec serialization
void to_json(nlohmann::json& j, const MaskPathKeyframeSpec& k);
void from_json(const nlohmann::json& j, MaskPathKeyframeSpec& k);

// AnimatedPropertySpec serialization
void to_json(nlohmann::json& j, const AnimatedScalarSpec& a);
void from_json(const nlohmann::json& j, AnimatedScalarSpec& a);

void to_json(nlohmann::json& j, const AnimatedVector2Spec& a);
void from_json(const nlohmann::json& j, AnimatedVector2Spec& a);

void to_json(nlohmann::json& j, const AnimatedVector3Spec& a);
void from_json(const nlohmann::json& j, AnimatedVector3Spec& a);

void to_json(nlohmann::json& j, const AnimatedColorSpec& a);
void from_json(const nlohmann::json& j, AnimatedColorSpec& a);

void to_json(nlohmann::json& j, const AnimatedMaskPathSpec& a);
void from_json(const nlohmann::json& j, AnimatedMaskPathSpec& a);

// AnimatedEffectSpec serialization
void to_json(nlohmann::json& j, const AnimatedEffectSpec& e);
void from_json(const nlohmann::json& j, AnimatedEffectSpec& e);



} // namespace tachyon
