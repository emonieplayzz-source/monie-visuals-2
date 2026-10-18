#version 120
// monie visuals - scene pass. 15 original procedural scenes, one shared parameter/audio system.
// Audio inputs a_* are already sensitivity-scaled and smoothed on the CPU. Params arrive as u_<id>.
uniform int   u_scene;
uniform float u_time;
uniform vec2  u_res;
uniform float a_bass, a_mid, a_treb, a_beat, a_rms, a_flux;
uniform sampler2D u_prev, u_imgA, u_imgB, u_spec;
uniform float u_hasImg, u_blend, u_aspA, u_aspB, u_fade;
uniform float u_extreme, u_zoom, u_rotation, u_symmetry, u_complexity, u_particles, u_distort, u_morph,
              u_feedback, u_cycle, u_reactcolor;
uniform vec3  u_c0, u_c1, u_c2;
uniform float u_rainbow;
varying vec2 vUV;

#define PI 3.14159265

float hash21(vec2 p) { p = fract(p * vec2(123.34, 456.21)); p += dot(p, p + 45.32); return fract(p.x * p.y); }
vec2  hash22(vec2 p) { float n = hash21(p); return vec2(n, hash21(p + n + 17.0)); }
float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash21(i), hash21(i + vec2(1.0, 0.0)), f.x), mix(hash21(i + vec2(0.0, 1.0)), hash21(i + vec2(1.0, 1.0)), f.x), f.y);
}
float fbm(vec2 p) { float s = 0.0, a = 0.5; for (int i = 0; i < 5; i++) { s += a * noise(p); p = p * 2.03 + vec2(7.1, 3.7); a *= 0.5; } return s; }
mat2 rot(float a) { float c = cos(a), s = sin(a); return mat2(c, -s, s, c); }
vec3 hsv2rgb(vec3 c) { vec3 p = abs(fract(c.xxx + vec3(1.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0); return c.z * mix(vec3(1.0), clamp(p - 1.0, 0.0, 1.0), c.y); }

vec3 pal(float t) {
    t = clamp(t, 0.0, 1.0);
    if (u_rainbow > 0.5)
        return hsv2rgb(vec3(fract(t * 0.8 + u_time * 0.05 * u_cycle + a_rms * u_reactcolor * 0.3), 0.85, 0.12 + 0.88 * smoothstep(0.0, 0.4, t)));
    return t < 0.5 ? mix(u_c0, u_c1, t * 2.0) : mix(u_c1, u_c2, t * 2.0 - 1.0);
}

// ---------- image access (cover-fit, mirrored edges, crossfade A->B) ----------
vec2 mirrorT(vec2 t) { return 1.0 - abs(1.0 - mod(t, 2.0)); }
vec3 demoImg(vec2 p) {
    float r = length(p), a = atan(p.y, p.x);
    float v = 0.5 + 0.5 * sin(r * 10.0 - u_time * 0.5) * cos(a * 5.0);
    return pal(0.5 * v + 0.5 * fbm(p * 3.0));
}
vec3 fetchImg(sampler2D s, float asp, vec2 p) {
    float a = u_res.x / u_res.y;
    float k = max(a / asp, 1.0);
    vec2 t = p / (2.0 * vec2(asp * k, k)) + 0.5;
    t.y = 1.0 - t.y;   // JUCE uploads top row first; if images appear upside-down on your driver, remove this line
    return texture2D(s, mirrorT(t)).rgb;
}
vec3 img(vec2 p) {
    if (u_hasImg < 0.5) return demoImg(p);
    vec3 A = fetchImg(u_imgA, u_aspA, p);
    if (u_blend > 0.002) return mix(A, fetchImg(u_imgB, u_aspB, p), u_blend);
    return A;
}

// ---------- particles ----------
vec3 particles(vec2 p, float amt) {
    vec3 c = vec3(0.0);
    for (int l = 0; l < 3; l++) {
        float fl = float(l);
        vec2 g = p * (7.0 + fl * 6.0) + vec2(sin(fl * 2.0), -u_time * (0.3 + 0.2 * fl) * (0.4 + a_treb * 1.5));
        vec2 id = floor(g), f = fract(g) - 0.5;
        float h = hash21(id + fl * 10.0);
        vec2 off = (hash22(id + fl) - 0.5) * 0.6;
        float size = 0.035 + 0.06 * h * (0.5 + a_treb * 2.0);
        float s = smoothstep(size, 0.0, length(f - off)) * step(h, amt * 0.55);
        c += pal(0.7 + h * 0.3) * s * (0.6 + 1.6 * a_treb);
    }
    return c;
}

// ---------- scene 0: liquid image morphing ----------
vec3 scene0(vec2 p) {
    float d = u_distort * (0.4 + u_extreme * 2.2), t = u_time;
    vec2 q = p;
    for (int i = 0; i < 4; i++) {
        float fi = float(i);
        q += d * (0.06 + 0.22 * a_bass) * vec2(sin(q.y * (2.0 + fi * 1.7) + t * (0.6 + 0.25 * fi) + a_bass * 4.0),
                                               cos(q.x * (2.3 + fi * 1.3) - t * (0.7 + 0.2 * fi) + a_mid * 3.0));
    }
    q += d * (0.02 + 0.12 * a_mid) * (vec2(fbm(q * 3.0 + t * 0.3), fbm(q * 3.0 + 7.0 - t * 0.3)) - 0.5) * 4.0;
    float s = (0.004 + 0.02 * a_treb) * (1.0 + u_extreme * 2.0);
    vec3 c = vec3(img(q + vec2(s, 0.0)).r, img(q).g, img(q - vec2(s, 0.0)).b);
    return c + a_treb * 0.12;
}
// ---------- 1: abstract fluid ----------
vec3 scene1(vec2 p) {
    float t = u_time * 0.3; vec2 q = p * 1.5;
    for (int i = 0; i < 4; i++) {
        q += (u_distort * (0.5 + u_extreme)) * 0.6 * (vec2(fbm(q + vec2(t, 1.7 * float(i)) + a_bass), fbm(q + vec2(5.2, 1.3 * float(i)) - t)) - 0.3);
        q = rot(0.4) * q;
    }
    float f = fbm(q * 1.2 + t);
    vec3 c = pal(smoothstep(0.2, 0.9, f + a_bass * 0.4));
    return c + pal(0.95) * pow(max(0.0, 1.0 - abs(f - 0.5) * 6.0), 3.0) * (0.3 + a_mid);
}
// ---------- 2: fractal tunnel ----------
vec3 scene2(vec2 p) {
    float r = length(p) + 1e-3, a = atan(p.y, p.x);
    vec2 uv = vec2(a / PI * (u_symmetry * 0.5), 0.5 / r + u_time * (0.3 + a_bass * 0.75));
    float v = pow(0.5 + 0.5 * sin(uv.x * PI * 2.0) * sin(uv.y * PI * 4.0), 1.5);
    float rings = smoothstep(0.0, 0.12, abs(fract(uv.y * 2.0) - 0.5));
    vec3 c = pal(v * 0.7 + a_mid * 0.3) * (0.5 + 0.5 * rings) * smoothstep(0.0, 0.6, r);
    return c * (0.6 + a_bass * 1.2) + pal(1.0) * a_beat * 0.25 * (1.0 - clamp(r, 0.0, 1.0));
}
// ---------- 3: wireframe geometry ----------
float sdSeg(vec2 p, vec2 a, vec2 b) { vec2 pa = p - a, ba = b - a; return length(pa - ba * clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0)); }
vec3 scene3(vec2 p) {
    vec3 c = vec3(0.0);
    float lw = 0.004 + 0.008 * a_treb;
    if (p.y < -0.05) {
        float z = 0.6 / (-p.y + 0.0001);
        vec2 g = vec2(p.x * z, z + u_time * (0.8 + a_bass * 2.0));
        vec2 f = min(fract(g), 1.0 - fract(g));
        c += pal(0.6) * smoothstep(0.05, 0.0, min(f.x, f.y)) * clamp(-p.y * 2.5, 0.0, 1.0) * (0.6 + a_mid);
    }
    float sides = 3.0 + floor(u_complexity * 5.0);
    for (int j = 0; j < 6; j++) {
        float fj = float(j), rad = 0.16 + 0.1 * fj + a_bass * 0.12, ang0 = u_time * (0.3 + 0.1 * fj) * (j - 2 * (j / 2) == 0 ? 1.0 : -1.0);
        float dmin = 10.0;
        for (int i = 0; i < 8; i++) {
            if (float(i) >= sides) break;
            float a1 = ang0 + 6.2831853 * float(i) / sides, a2 = ang0 + 6.2831853 * float(i + 1) / sides;
            dmin = min(dmin, sdSeg(p - vec2(0.0, 0.1), rad * vec2(cos(a1), sin(a1)), rad * vec2(cos(a2), sin(a2))));
        }
        c += pal(0.4 + 0.1 * fj) * smoothstep(lw, 0.0, dmin) * 1.3 + pal(0.8) * smoothstep(lw * 5.0, 0.0, dmin) * 0.15;
    }
    return c;
}
// ---------- 4: plasma ----------
vec3 scene4(vec2 p) {
    float t = u_time; vec2 q = p * (2.0 + u_complexity * 3.0);
    float v = sin(q.x + t) + sin(q.y * 1.3 + t * 1.2) + sin((q.x + q.y) * 0.8 + t * 0.7) + sin(length(q) * 2.0 - t * 1.5 + a_bass * 6.0);
    v += a_mid * 2.0 * sin(q.x * q.y * 0.5);
    return pal(fract((v * 0.125 + 0.5) * (1.0 + a_treb)));
}
// ---------- 5: nebula + particles ----------
vec3 scene5(vec2 p) {
    float t = u_time * 0.1;
    float n = pow(clamp(fbm(p * 2.4 + t) * fbm(p * 3.6 - t * 1.3 + 4.0) * 2.0, 0.0, 1.0), 1.5 + u_extreme);
    return pal(n) * (0.5 + a_bass) + particles(p, max(u_particles, 0.5) * 1.4);
}
// ---------- 6: radial spectrum ----------
vec3 scene6(vec2 p) {
    float r = length(p), x = abs(atan(p.y, p.x)) / PI;
    float v = texture2D(u_spec, vec2(x * 0.98 + 0.01, 0.25)).r;
    float base = 0.25 + a_bass * 0.08, bar = base + v * 0.55 * (0.5 + u_extreme);
    float cell = fract(x * 64.0), gap = step(0.15, cell) * step(cell, 0.85);
    float inside = smoothstep(0.004, 0.0, r - bar) * smoothstep(base - 0.01, base, r) * gap;
    float ring = smoothstep(0.01 + a_beat * 0.02, 0.0, abs(r - base));
    return pal(0.3 + v * 0.7) * inside * 1.3 + pal(0.9) * ring * (0.5 + a_beat) + pal(0.2) * exp(-r * 2.0) * 0.3 * (a_bass + 0.2);
}
// ---------- 7: typography + geometric forms ("MONIE" drawn from line segments) ----------
float letterD(int id, vec2 q) {
    float d = 10.0;
    if (id == 0) { d = min(min(sdSeg(q, vec2(0,0), vec2(0,1)), sdSeg(q, vec2(0,1), vec2(.5,.45))), min(sdSeg(q, vec2(.5,.45), vec2(1,1)), sdSeg(q, vec2(1,1), vec2(1,0)))); }
    else if (id == 1) { d = min(min(sdSeg(q, vec2(0,0), vec2(0,1)), sdSeg(q, vec2(0,1), vec2(1,1))), min(sdSeg(q, vec2(1,1), vec2(1,0)), sdSeg(q, vec2(1,0), vec2(0,0)))); }
    else if (id == 2) { d = min(min(sdSeg(q, vec2(0,0), vec2(0,1)), sdSeg(q, vec2(0,1), vec2(1,0))), sdSeg(q, vec2(1,0), vec2(1,1))); }
    else if (id == 3) { d = min(sdSeg(q, vec2(.5,0), vec2(.5,1)), min(sdSeg(q, vec2(.2,1), vec2(.8,1)), sdSeg(q, vec2(.2,0), vec2(.8,0)))); }
    else { d = min(min(sdSeg(q, vec2(0,0), vec2(0,1)), sdSeg(q, vec2(0,1), vec2(1,1))), min(sdSeg(q, vec2(0,.5), vec2(.8,.5)), sdSeg(q, vec2(0,0), vec2(1,0)))); }
    return d;
}
vec3 scene7(vec2 p) {
    vec3 c = vec3(0.0);
    for (int k = 0; k < 4; k++) {
        float fk = float(k);
        vec2 q = rot(u_time * (0.2 + 0.1 * fk) * (1.0 + a_mid)) * p;
        float d = abs(max(abs(q.x), abs(q.y)) - (0.3 + 0.18 * fk + a_bass * 0.15));
        c += pal(0.3 + 0.15 * fk) * smoothstep(0.006 + a_treb * 0.01, 0.0, d) * 0.7;
    }
    float h = 0.3 + 0.25 * a_bass;
    for (int i = 0; i < 5; i++) {
        float fi = float(i);
        vec2 o = vec2((fi - 2.0) * 0.36 - 0.14, -h * 0.5);
        o += (vec2(noise(vec2(fi, u_time * 2.0)), noise(vec2(fi + 9.0, u_time * 2.0))) - 0.5) * a_mid * 0.15;
        vec2 q = (p - o) / vec2(0.26, h);
        float d = letterD(i, q) * 0.28;
        float w = 0.012 + a_treb * 0.02;
        c += pal(0.55 + 0.1 * fi) * (smoothstep(w, 0.0, d) * 1.4 + smoothstep(w * 6.0, 0.0, d) * 0.2);
    }
    return c;
}
// ---------- 8: digital feedback (the feedback buffer does the heavy lifting) ----------
vec3 scene8(vec2 p) {
    vec2 q = rot(u_time * 0.5 + a_bass * 2.0) * p;
    float d = abs(max(abs(q.x), abs(q.y)) - (0.25 + 0.1 * sin(u_time) + a_bass * 0.2));
    float ring = smoothstep(0.01, 0.0, abs(length(p) - 0.4 - a_mid * 0.2));
    return pal(0.6 + 0.4 * sin(u_time)) * smoothstep(0.015 + a_treb * 0.03, 0.0, d) * 1.5 + pal(0.9) * ring;
}
// ---------- 9: kaleidoscope ----------
vec3 scene9(vec2 p) {
    float a = atan(p.y, p.x), r = length(p), seg = 2.0 * PI / max(u_symmetry, 1.0);
    a = mod(a, seg); a = abs(a - seg * 0.5);
    vec2 q = rot(u_time * 0.1) * (r * vec2(cos(a), sin(a)));
    vec3 c;
    if (u_hasImg > 0.5) {
        q += (u_distort * 0.1) * vec2(sin(q.y * 6.0 + u_time + a_bass * 3.0), cos(q.x * 6.0 - u_time));
        c = mix(pal(fbm(q * 3.0 + u_time * 0.2)), img(q * 1.2), u_morph);
    } else c = pal(fbm(q * 3.0 + u_time * 0.2 + a_mid) + a_bass * 0.3);
    return c * (0.7 + a_treb * 0.6);
}
// ---------- 10: glitch + RGB separation ----------
vec3 scene10(vec2 p) {
    float t = floor(u_time * (6.0 + a_treb * 20.0)), g = a_beat + a_flux * 0.6, ex = 1.0 + u_extreme * 2.0;
    vec2 q = p;
    float row = floor((p.y + 1.0) * (10.0 + 20.0 * hash21(vec2(t, 1.0)))), h = hash21(vec2(row, t));
    if (h > 1.0 - 0.25 * (0.3 + g) * (0.5 + u_extreme * 2.0)) q.x += (hash21(vec2(t, row)) - 0.5) * 0.6 * (0.3 + g) * ex;
    float blk = hash21(floor(p * vec2(6.0, 4.0)) + t);
    if (blk > 0.93 - 0.05 * g) q += (hash22(vec2(t, blk)) - 0.5) * 0.2;
    float s = (0.01 + 0.04 * g) * ex;
    vec3 c = vec3(img(q + vec2(s, 0.0)).r, img(q).g, img(q - vec2(s, 0.0)).b);
    c *= 0.9 + 0.1 * sin(p.y * u_res.y * 1.5);
    if (hash21(vec2(row, t + 3.0)) > 0.97) c = 1.0 - c;
    return c;
}
// ---------- 11: waveform / oscilloscope ----------
vec3 scene11(vec2 p) {
    float x = clamp((p.x / (u_res.x / u_res.y)) * 0.5 + 0.5, 0.0, 1.0);
    float w = texture2D(u_spec, vec2(x, 0.75)).r * 2.0 - 1.0;
    float y = w * (0.35 + 0.5 * a_rms) * (0.5 + u_extreme) * 2.0;
    float d = abs(p.y - y), lw = 0.006 + 0.01 * a_beat;
    vec3 c = pal(0.8) * smoothstep(lw * 3.0, 0.0, d) * 0.5 + pal(1.0) * smoothstep(lw, 0.0, d) * 1.5;
    c += pal(0.6) * smoothstep(lw, 0.0, abs(p.y + y * 0.6)) * 0.6;
    vec2 gg = min(fract(p * 4.0 + 0.5), 1.0 - fract(p * 4.0 + 0.5));
    return c + pal(0.3) * smoothstep(0.012, 0.0, min(gg.x, gg.y)) * 0.12;
}
// ---------- 12: melting image ----------
vec3 scene12(vec2 p) {
    float m = (0.1 + 0.9 * u_distort) * (0.4 + a_bass * 1.5) * (0.5 + u_extreme * 2.0);
    vec2 q = p;
    float drip = fbm(vec2(p.x * 3.0, u_time * 0.2)) * (0.5 + 0.5 * sin(p.x * 7.0 + u_time));
    q.y += m * (drip * (p.y + 1.2) * 0.5 + 0.1 * fbm(vec2(p.x * 8.0, p.y * 2.0 - u_time * 0.5)));
    q.x += m * 0.05 * sin(p.y * 8.0 + u_time * 2.0 + a_mid * 5.0);
    q.y += max(0.0, -p.y * 0.5 + 0.3) * m * 0.25;
    vec3 c = img(q);
    return mix(c, c.gbr * 0.5 + c * 0.7, smoothstep(0.6, 1.0, fbm(q * 4.0 + u_time * 0.1)) * a_mid * 0.5);
}
// ---------- 13: fractal / recursive (KIFS) ----------
vec3 scene13(vec2 p) {
    vec2 q = p * (1.4 - 0.3 * u_complexity);
    float acc = 0.0, t = u_time * 0.15; vec3 c = vec3(0.0);
    for (int i = 0; i < 7; i++) {
        q = abs(q) - vec2(0.45 + 0.15 * sin(t + a_bass), 0.3 + 0.1 * cos(t * 1.3));
        q = rot(t + float(i) * 0.3 + a_mid * 0.5) * q;
        q *= 1.25 + 0.1 * a_bass;
        float d = length(q);
        acc += exp(-d * 3.0);
        c += pal(fract(float(i) * 0.13 + d * 0.5 + t)) * exp(-d * (2.5 + u_extreme * 2.0));
    }
    return c * 0.35 * (0.6 + a_treb) + pal(0.9) * smoothstep(0.0, 3.0, acc) * 0.2;
}
// ---------- 14: procedural 3D-style forms (raymarched) ----------
float map3(vec3 p) {
    p.xz = rot(u_time * 0.3) * p.xz; p.xy = rot(u_time * 0.2) * p.xy;
    float fq = 4.0 + u_complexity * 6.0;
    float s = length(p) - (0.7 + a_bass * 0.25);
    s += sin(p.x * fq + u_time) * sin(p.y * fq) * sin(p.z * 5.0 - u_time) * (0.08 + 0.18 * a_mid) * (0.5 + u_extreme * 2.0);
    float tor = length(vec2(length(p.xz) - 1.15, p.y)) - (0.08 + a_treb * 0.07);
    return min(s, tor);
}
vec3 scene14(vec2 p) {
    vec3 ro = vec3(0.0, 0.0, 3.2), rd = normalize(vec3(p, -1.8));
    float t = 0.0;
    for (int i = 0; i < 48; i++) { float d = map3(ro + rd * t); if (d < 0.002 || t > 8.0) break; t += d; }
    vec3 c = pal(0.1) * (1.0 - length(p) * 0.4) * 0.4;
    if (t < 8.0) {
        vec3 pos = ro + rd * t; vec2 e = vec2(0.003, 0.0);
        vec3 n = normalize(vec3(map3(pos + e.xyy) - map3(pos - e.xyy), map3(pos + e.yxy) - map3(pos - e.yxy), map3(pos + e.yyx) - map3(pos - e.yyx)));
        vec3 l = normalize(vec3(0.6, 0.8, 0.7));
        float dif = max(dot(n, l), 0.0), fr = pow(1.0 - max(dot(n, -rd), 0.0), 3.0), sp = pow(max(dot(reflect(-l, n), -rd), 0.0), 24.0);
        c = pal(0.3 + 0.4 * dif) * (0.2 + dif) + pal(1.0) * fr * (0.6 + a_treb) + sp * 0.8;
    }
    return c;
}

void main() {
    float asp = u_res.x / u_res.y;
    vec2 p = (vUV * 2.0 - 1.0) * vec2(asp, 1.0);
    p = rot(u_rotation) * p / max(u_zoom, 0.05);

    vec3 c;
    if      (u_scene == 0)  c = scene0(p);   else if (u_scene == 1)  c = scene1(p);
    else if (u_scene == 2)  c = scene2(p);   else if (u_scene == 3)  c = scene3(p);
    else if (u_scene == 4)  c = scene4(p);   else if (u_scene == 5)  c = scene5(p);
    else if (u_scene == 6)  c = scene6(p);   else if (u_scene == 7)  c = scene7(p);
    else if (u_scene == 8)  c = scene8(p);   else if (u_scene == 9)  c = scene9(p);
    else if (u_scene == 10) c = scene10(p);  else if (u_scene == 11) c = scene11(p);
    else if (u_scene == 12) c = scene12(p);  else if (u_scene == 13) c = scene13(p);
    else                    c = scene14(p);

    // image layer on top of procedural scenes (audio-warped, luminance-modulated)
    if (u_hasImg > 0.5 && u_morph > 0.01 && u_scene != 0 && u_scene != 9 && u_scene != 10 && u_scene != 12) {
        vec2 q = p + u_distort * 0.05 * vec2(sin(p.y * 5.0 + u_time), cos(p.x * 5.0 - u_time)) * (0.5 + a_mid);
        float l = dot(c, vec3(0.333));
        c = mix(c, img(q) * (0.4 + l * 1.4), u_morph * 0.55);
    }
    if (u_particles > 0.01 && u_scene != 5) c += particles(p, u_particles * 0.8);

    // feedback / trails (ping-pong buffer)
    float fbAmt = u_feedback, zf = 1.0 / (1.004 + 0.012 * a_bass), rf = 0.004 * sin(u_time * 0.3);
    if (u_scene == 8) { fbAmt = max(fbAmt, 0.9); zf = 1.0 / (1.02 + 0.02 * a_bass); rf = 0.03; }
    vec2 fp = (vUV - 0.5) * vec2(asp, 1.0);
    fp = rot(rf) * fp * zf;
    fp += u_distort * 0.0015 * (vec2(noise(fp * 20.0 + u_time), noise(fp * 20.0 - u_time)) - 0.5) * (1.0 + a_mid * 3.0);
    vec3 prev = texture2D(u_prev, fp / vec2(asp, 1.0) + 0.5).rgb;
    c = max(c, prev * fbAmt);

    // scene crossfade: blend from last frame of the previous scene
    c = mix(c, texture2D(u_prev, vUV).rgb, clamp(u_fade, 0.0, 1.0));
    gl_FragColor = vec4(c, 1.0);
}
