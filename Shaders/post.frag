#version 120
// monie visuals - post pass: chromatic aberration, glow, edge, gradient map, color grade, posterize/threshold, CRT/film.
uniform sampler2D u_tex;
uniform vec2  u_res;
uniform float u_time;
uniform float a_bass, a_mid, a_treb, a_beat, a_rms;
uniform float u_master, u_rgb, u_glow, u_grain, u_vignette, u_scanlines, u_pixel, u_edge, u_poster, u_thresh, u_invert,
              u_hue, u_sat, u_bright, u_contrast, u_gamma, u_temp, u_tint, u_cycle, u_reactcolor, u_palmix, u_bgcolor;
uniform vec3  u_c0, u_c1, u_c2;
varying vec2 vUV;

float hash(vec2 p) { p = fract(p * vec2(443.897, 441.423)); p += dot(p, p + 19.19); return fract(p.x * p.y); }
float lum(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }
vec3 hueShift(vec3 c, float h) {
    float a = h * 6.2831853, ca = cos(a), sa = sin(a); vec3 k = vec3(0.57735);
    return c * ca + cross(k, c) * sa + k * dot(k, c) * (1.0 - ca);
}
void main() {
    float asp = u_res.x / u_res.y;
    vec2 uv = vUV;
    if (u_pixel > 0.001) { float cells = mix(900.0, 24.0, sqrt(u_pixel)); vec2 g = vec2(cells * asp, cells); uv = (floor(uv * g) + 0.5) / g; }
    vec2 d = uv - 0.5;
    vec2 sh = d * u_rgb * (0.02 + 0.03 * a_treb + 0.02 * a_beat);
    vec3 c = vec3(texture2D(u_tex, uv + sh).r, texture2D(u_tex, uv).g, texture2D(u_tex, uv - sh).b);

    if (u_glow > 0.01) {
        vec3 b = vec3(0.0); float rad = 0.004 + 0.012 * u_glow;
        for (int i = 0; i < 8; i++) {
            float an = float(i) * 0.785398; vec2 o = vec2(cos(an), sin(an)) * rad * vec2(1.0, asp);
            b += max(texture2D(u_tex, uv + o).rgb - 0.45, 0.0) + max(texture2D(u_tex, uv + o * 2.2).rgb - 0.45, 0.0);
        }
        c += b / 16.0 * u_glow * (1.5 + a_beat);
    }
    if (u_edge > 0.01) {
        vec2 px = 1.0 / u_res;
        float tl = lum(texture2D(u_tex, uv + vec2(-px.x, px.y)).rgb), t = lum(texture2D(u_tex, uv + vec2(0.0, px.y)).rgb), tr = lum(texture2D(u_tex, uv + px).rgb);
        float l = lum(texture2D(u_tex, uv + vec2(-px.x, 0.0)).rgb), r = lum(texture2D(u_tex, uv + vec2(px.x, 0.0)).rgb);
        float bl = lum(texture2D(u_tex, uv - px).rgb), b2 = lum(texture2D(u_tex, uv + vec2(0.0, -px.y)).rgb), br = lum(texture2D(u_tex, uv + vec2(px.x, -px.y)).rgb);
        float gx = -tl - 2.0 * l - bl + tr + 2.0 * r + br, gy = -tl - 2.0 * t - tr + bl + 2.0 * b2 + br;
        float e = clamp(length(vec2(gx, gy)) * 2.0, 0.0, 1.0);
        c = mix(c, u_c2 * e * 2.0 + c * 0.15, u_edge);
    }
    if (u_palmix > 0.01) {
        float l = lum(c);
        c = mix(c, l < 0.5 ? mix(u_c0, u_c1, l * 2.0) : mix(u_c1, u_c2, l * 2.0 - 1.0), u_palmix);
    }
    c.r += u_temp * 0.15; c.b -= u_temp * 0.15; c.g += u_tint * 0.12; c.r -= u_tint * 0.05; c.b -= u_tint * 0.05;
    c = hueShift(c, u_hue + u_time * 0.03 * u_cycle + a_rms * u_reactcolor * 0.25 + a_beat * u_reactcolor * 0.04);
    c = mix(vec3(lum(c)), c, u_sat);
    c = (c - 0.5) * u_contrast + 0.5;
    c *= u_bright * mix(0.35, 1.1, u_master);
    c = pow(max(c, 0.0), vec3(1.0 / u_gamma));
    if (u_poster > 0.01) { float lv = mix(24.0, 2.0, u_poster); c = floor(c * lv + 0.5) / lv; }
    if (u_thresh > 0.01) { float l = lum(c); c = mix(c, mix(u_c0, u_c2, step(mix(0.9, 0.2, u_thresh), l)), min(1.0, u_thresh * 4.0)); }
    c = mix(c, 1.0 - c, u_invert);
    c += u_c0 * u_bgcolor * (1.0 - lum(c));
    c *= 1.0 - u_scanlines * 0.35 * (0.5 + 0.5 * sin(vUV.y * u_res.y * 3.14159));
    c *= 1.0 - u_vignette * smoothstep(0.25, 0.9, length(d * vec2(asp, 1.0)) * 1.2);
    c += (hash(vUV * u_res + u_time * 61.0) - 0.5) * u_grain * 0.25;
    gl_FragColor = vec4(clamp(c, 0.0, 1.0), 1.0);
}
