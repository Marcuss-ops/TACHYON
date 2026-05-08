// Normal pack: Directional Blur Wipe
vec4 blur_x(sampler2D tex, vec2 uv, float radius) {
    vec4 sum = vec4(0.0);
    sum += texture(tex, uv - vec2(radius * 2.0, 0.0)) * 0.1;
    sum += texture(tex, uv - vec2(radius, 0.0)) * 0.2;
    sum += texture(tex, uv) * 0.4;
    sum += texture(tex, uv + vec2(radius, 0.0)) * 0.2;
    sum += texture(tex, uv + vec2(radius * 2.0, 0.0)) * 0.1;
    return sum;
}
vec4 transition(vec2 uv, sampler2D a, sampler2D b, float t) {
    float k = clamp(t, 0.0, 1.0);
    vec4 ca = blur_x(a, vec2(uv.x + (1.0 - k), uv.y), 0.01 * (1.0 - k));
    vec4 cb = blur_x(b, vec2(uv.x - k, uv.y), 0.01 * k);
    return mix(ca, cb, k);
}

