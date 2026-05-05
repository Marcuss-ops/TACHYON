// Normal pack: Zoom In
vec4 transition(vec2 uv, sampler2D a, sampler2D b, float t) {
    float k = clamp(t, 0.0, 0.999);
    vec2 center = vec2(0.5);
    vec2 za = center + (uv - center) / (1.0 - k * 0.25);
    vec2 zb = center + (uv - center) * (1.0 - k);
    return mix(texture(a, za), texture(b, zb), k);
}

