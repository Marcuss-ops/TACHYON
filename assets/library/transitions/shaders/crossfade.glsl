// Normal pack: Crossfade
vec4 transition(vec2 uv, sampler2D a, sampler2D b, float t) {
    vec4 ca = texture(a, uv);
    vec4 cb = texture(b, uv);
    return mix(ca, cb, clamp(t, 0.0, 1.0));
}

