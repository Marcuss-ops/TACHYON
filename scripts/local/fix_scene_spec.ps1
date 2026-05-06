$content = Get-Content src/core/spec/scene_spec_json.cpp -Raw
$pattern = '(?s)bool parse_keyframe_common\(const json& object, KeyframeT& keyframe, const std::string& path, DiagnosticBag& diagnostics\) \{.*?return true;\r?\n\}'
$replacement = 'bool parse_keyframe_common(const json& object, KeyframeT& keyframe, const std::string& path, DiagnosticBag& diagnostics) {
    if (!object.contains("time") || !object.at("time").is_number()) {
        diagnostics.add_error("scene.layer.keyframe.time_invalid", "keyframe.time must be numeric", path + ".time");
        return false;
    }
    keyframe.time = object.at("time").get<double>();
    if (object.contains("easing")) {
        keyframe.easing = parse_easing_preset(object.at("easing"));
        if (keyframe.easing == animation::EasingPreset::Custom) {
            if (object.contains("bezier")) {
                keyframe.bezier = parse_bezier(object.at("bezier"));
            } else {
                if (object.contains("speed_in")) keyframe.speed_in = object.at("speed_in").get<double>();
                if (object.contains("influence_in")) keyframe.influence_in = object.at("influence_in").get<double>();
                if (object.contains("speed_out")) keyframe.speed_out = object.at("speed_out").get<double>();
                if (object.contains("influence_out")) keyframe.influence_out = object.at("influence_out").get<double>();
            }
        }
    }
    return true;
}'
$content = $content -replace $pattern, $replacement
Set-Content src/core/spec/scene_spec_json.cpp $content -NoNewline
