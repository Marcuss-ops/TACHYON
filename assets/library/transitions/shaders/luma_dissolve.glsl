// Normal pack: Luma Dissolve
float luma(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }
float noise(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453); }
vec4 transition(vec2 uv, sampler2D a, sampler2D b, float t) {
    float k = clamp(t, 0.0, 1.0);
    vec4 ca = texture(a, uv);
    vec4 cb = texture(b, uv);
    float m = noise(uv * 64.0);
    return (m < k) ? cb : ca;
}

