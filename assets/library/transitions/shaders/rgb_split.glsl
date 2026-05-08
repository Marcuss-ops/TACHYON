// Normal pack: RGB Split
vec4 transition(vec2 uv, sampler2D a, sampler2D b, float t) {
    float k = clamp(t, 0.0, 1.0) * 0.02;
    vec4 cb;
    cb.r = texture(b, uv + vec2(k, 0.0)).r;
    cb.g = texture(b, uv).g;
    cb.b = texture(b, uv - vec2(k, 0.0)).b;
    cb.a = texture(b, uv).a;
    return mix(texture(a, uv), cb, clamp(t, 0.0, 1.0));
}

