// Normal pack: Glitch Slice
float hash12(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
vec4 transition(vec2 uv, sampler2D a, sampler2D b, float t) {
    float k = clamp(t, 0.0, 1.0);
    float slice = floor(uv.y * 20.0);
    float jitter = (hash12(vec2(slice, floor(k * 10.0))) - 0.5) * 0.04;
    vec2 puv = vec2(uv.x + jitter, uv.y);
    return mix(texture(a, puv), texture(b, puv), step(0.5, k));
}

