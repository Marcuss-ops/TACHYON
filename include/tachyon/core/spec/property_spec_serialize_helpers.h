#pragma once

#include "tachyon/core/spec/schema/properties/property_spec.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace tachyon {

// WiggleSpec serialization (used by multiple property types)
void to_json(json& j, const WiggleSpec& w);
void from_json(const json& j, WiggleSpec& w);

// ScalarKeyframeSpec serialization
void to_json(json& j, const ScalarKeyframeSpec& k);
void from_json(const json& j, ScalarKeyframeSpec& k);

// Vector2KeyframeSpec serialization  
void to_json(json& j, const Vector2KeyframeSpec& k);
void from_json(const json& j, Vector2KeyframeSpec& k);

// Vector3KeyframeSpec::Keyframe serialization
void to_json(json& j, const AnimatedVector3Spec::Keyframe& k);
void from_json(const json& j, AnimatedVector3Spec::Keyframe& k);

// ColorKeyframeSpec serialization
void to_json(json& j, const ColorKeyframeSpec& k);
void from_json(const json& j, ColorKeyframeSpec& k);

// MaskPathKeyframeSpec serialization
void to_json(json& j, const MaskPathKeyframeSpec& k);
void from_json(const json& j, MaskPathKeyframeSpec& k);

// AnimatedPropertySpec serialization
void to_json(json& j, const AnimatedScalarSpec& a);
void from_json(const json& j, AnimatedScalarSpec& a);

void to_json(json& j, const AnimatedVector2Spec& a);
void from_json(const json& j, AnimatedVector2Spec& a);

void to_json(json& j, const AnimatedVector3Spec& a);
void from_json(const json& j, AnimatedVector3Spec& a);

void to_json(json& j, const AnimatedColorSpec& a);
void from_json(const json& j, AnimatedColorSpec& a);

void to_json(json& j, const AnimatedMaskPathSpec& a);
void from_json(const json& j, AnimatedMaskPathSpec& a);

// AnimatedEffectSpec serialization
void to_json(json& j, const AnimatedEffectSpec& e);
void from_json(const json& j, AnimatedEffectSpec& e);



} // namespace tachyon
