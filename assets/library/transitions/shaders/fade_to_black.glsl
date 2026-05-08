// Normal pack: Fade to Black
vec4 transition(vec2 uv, sampler2D a, sampler2D b, float t) {
    vec4 ca = texture(a, uv);
    vec4 cb = texture(b, uv);
    float k = clamp(t, 0.0, 1.0);
    float fb = smoothstep(0.0, 0.5, k);
    vec4 black = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 mid = mix(ca, black, fb);
    return mix(mid, cb, smoothstep(0.5, 1.0, k));
}

