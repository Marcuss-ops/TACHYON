// Normal pack: Zoom Blur
vec4 transition(vec2 uv, sampler2D a, sampler2D b, float t) {
    float k = clamp(t, 0.0, 1.0);
    vec2 c = vec2(0.5);
    vec4 acc = vec4(0.0);
    for (int i = 0; i < 6; ++i) {
        float f = float(i) / 5.0;
        vec2 p = mix(uv, c, f * k);
        acc += texture(b, p);
    }
    return mix(texture(a, uv), acc / 6.0, k);
}

