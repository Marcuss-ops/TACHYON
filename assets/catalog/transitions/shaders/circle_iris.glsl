// Normal pack: Circle Iris
vec4 transition(vec2 uv, sampler2D a, sampler2D b, float t) {
    float d = length(uv - vec2(0.5));
    return (d < clamp(t, 0.0, 1.0) * 0.75) ? texture(b, uv) : texture(a, uv);
}

