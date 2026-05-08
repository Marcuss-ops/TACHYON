// Normal pack: Spin
mat2 rot(float a) { float s = sin(a); float c = cos(a); return mat2(c, -s, s, c); }
vec4 transition(vec2 uv, sampler2D a, sampler2D b, float t) {
    vec2 c = vec2(0.5);
    float k = clamp(t, 0.0, 1.0);
    vec2 pa = c + rot(k * 3.1415926535) * (uv - c);
    vec2 pb = c + rot((1.0 - k) * 3.1415926535) * (uv - c);
    return mix(texture(a, pa), texture(b, pb), k);
}

