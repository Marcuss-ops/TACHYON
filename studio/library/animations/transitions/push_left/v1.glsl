// Normal pack: Push Left
vec4 transition(vec2 uv, sampler2D a, sampler2D b, float t) {
    float k = clamp(t, 0.0, 1.0);
    vec2 ua = vec2(uv.x + k, uv.y);
    vec2 ub = vec2(uv.x + k - 1.0, uv.y);
    vec4 ca = texture(a, ua);
    vec4 cb = texture(b, ub);
    return mix(ca, cb, step(1.0, uv.x + k));
}

