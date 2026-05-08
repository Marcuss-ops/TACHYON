// Normal pack: Pixelate
vec4 transition(vec2 uv, sampler2D a, sampler2D b, float t) {
    float blocks = mix(20.0, 1.0, clamp(t, 0.0, 1.0));
    vec2 p = floor(uv * blocks) / max(blocks, 1.0);
    return mix(texture(a, p), texture(b, p), clamp(t, 0.0, 1.0));
}

