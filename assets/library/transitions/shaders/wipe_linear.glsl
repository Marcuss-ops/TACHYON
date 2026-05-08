// Normal pack: Linear Wipe
vec4 transition(vec2 uv, sampler2D a, sampler2D b, float t) {
    float edge = clamp(t, 0.0, 1.0);
    return (uv.x < edge) ? texture(b, uv) : texture(a, uv);
}

