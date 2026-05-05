// Normal pack: Slide with easing
float ease(float t) { return t * t * (3.0 - 2.0 * t); }
vec4 transition(vec2 uv, sampler2D a, sampler2D b, float t) {
    float k = ease(clamp(t, 0.0, 1.0));
    vec4 ca = texture(a, vec2(uv.x - k, uv.y));
    vec4 cb = texture(b, vec2(uv.x + 1.0 - k, uv.y));
    return mix(ca, cb, k);
}

