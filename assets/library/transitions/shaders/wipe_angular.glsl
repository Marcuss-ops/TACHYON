// Normal pack: Angular Wipe
const float PI2 = 6.28318530718;
vec4 transition(vec2 uv, sampler2D a, sampler2D b, float t) {
    vec2 p = uv - vec2(0.5);
    float ang = atan(p.y, p.x) + 3.1415926535;
    float edge = clamp(t, 0.0, 1.0) * PI2;
    return (ang < edge) ? texture(b, uv) : texture(a, uv);
}

