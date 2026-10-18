// monie visuals - single source of truth for parameters. The processor builds the APVTS layout from this
// table, the editor builds sliders from it, and the renderer sends every entry to the shaders as "u_<id>".
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace monie
{
enum Group : unsigned { GVis = 1, GImg = 2, GFx = 4, GCol = 8, GCus = 16, GAud = 32 };

struct ParamDef { const char* id; const char* name; float minV, maxV, defV, step; unsigned groups; bool toggle; };

inline constexpr ParamDef kParams[] =
{
    // id          name                       min    max    def   step  groups
    { "master",    "Master Intensity",        0.f,   1.f,   .85f, 0.f,  GVis | GCus, false },
    { "speed",     "Animation Speed",         0.f,   3.f,   1.f,  0.f,  GVis | GCus, false },
    { "extreme",   "Subtle - Extreme",        0.f,   1.f,   .35f, 0.f,  GVis | GImg | GCus, false },
    { "zoom",      "Zoom",                    .25f,  4.f,   1.f,  0.f,  GVis | GCus, false },
    { "rotation",  "Rotation",               -3.14159f, 3.14159f, 0.f, 0.f, GVis | GCus, false },
    { "symmetry",  "Symmetry / Repeats",      1.f,   16.f,  6.f,  1.f,  GVis | GCus, false },
    { "complexity","Shape Complexity",        0.f,   1.f,   .5f,  0.f,  GVis | GCus, false },
    { "particles", "Particle Density",        0.f,   1.f,   .35f, 0.f,  GVis | GFx | GCus, false },
    { "distort",   "Distortion Amount",       0.f,   2.f,   .7f,  0.f,  GImg | GCus, false },
    { "morph",     "Image Morph Mix",         0.f,   1.f,   .8f,  0.f,  GImg | GCus, false },
    { "imgspeed",  "Image Transition Speed",  .05f,  4.f,   .8f,  0.f,  GImg | GCus, false },
    { "xfade",     "Scene Crossfade Time",    .05f,  4.f,   .8f,  0.f,  GVis | GCus, false },
    { "feedback",  "Feedback / Trails",       0.f,   .97f,  .4f,  0.f,  GFx | GCus, false },
    { "rgb",       "RGB Separation",          0.f,   1.f,   .2f,  0.f,  GFx | GCus, false },
    { "glow",      "Glow / Bloom",            0.f,   2.f,   .6f,  0.f,  GFx | GCus, false },
    { "grain",     "Grain",                   0.f,   1.f,   .1f,  0.f,  GFx | GCus, false },
    { "vignette",  "Vignette",                0.f,   1.f,   .4f,  0.f,  GFx | GCus, false },
    { "scanlines", "Scanlines",               0.f,   1.f,   0.f,  0.f,  GFx | GCus, false },
    { "pixel",     "Pixelation",              0.f,   1.f,   0.f,  0.f,  GFx | GCus, false },
    { "edge",      "Edge Detection",          0.f,   1.f,   0.f,  0.f,  GFx | GCus, false },
    { "poster",    "Posterize",               0.f,   1.f,   0.f,  0.f,  GFx | GCol | GCus, false },
    { "thresh",    "Threshold",               0.f,   1.f,   0.f,  0.f,  GFx | GCus, false },
    { "invert",    "Invert Colors",           0.f,   1.f,   0.f,  1.f,  GFx | GCol | GCus, true  },
    { "hue",       "Hue Shift",               0.f,   1.f,   0.f,  0.f,  GCol | GCus, false },
    { "sat",       "Saturation",              0.f,   2.f,   1.f,  0.f,  GCol | GCus, false },
    { "bright",    "Brightness",              0.f,   2.f,   1.f,  0.f,  GCol | GCus, false },
    { "contrast",  "Contrast",                0.f,   2.f,   1.f,  0.f,  GCol | GCus, false },
    { "gamma",     "Gamma",                   .4f,   2.5f,  1.f,  0.f,  GCol | GCus, false },
    { "temp",      "Temperature",            -1.f,   1.f,   0.f,  0.f,  GCol | GCus, false },
    { "tint",      "Tint",                   -1.f,   1.f,   0.f,  0.f,  GCol | GCus, false },
    { "cycle",     "Color Cycling Speed",     0.f,   2.f,   0.f,  0.f,  GCol | GCus, false },
    { "reactcolor","Audio Color Reactivity",  0.f,   1.f,   .4f,  0.f,  GCol | GCus, false },
    { "palmix",    "Gradient Map (images)",   0.f,   1.f,   0.f,  0.f,  GCol | GCus, false },
    { "bgcolor",   "Background Level",        0.f,   1.f,   0.f,  0.f,  GCol | GCus, false },
    { "bass",      "Bass Sensitivity",        0.f,   3.f,   1.f,  0.f,  GAud | GCus, false },
    { "mid",       "Mid Sensitivity",         0.f,   3.f,   1.f,  0.f,  GAud | GCus, false },
    { "treble",    "Treble Sensitivity",      0.f,   3.f,   1.f,  0.f,  GAud | GCus, false },
    { "beat",      "Beat Sensitivity",        0.f,   3.f,   1.f,  0.f,  GAud | GCus, false },
    { "smooth",    "Smoothing",               0.f,   1.f,   .5f,  0.f,  GAud | GCus, false },
    { "attack",    "Attack",                  0.f,   1.f,   .7f,  0.f,  GAud | GCus, false },
    { "release",   "Release",                 0.f,   1.f,   .4f,  0.f,  GAud | GCus, false },
};
inline constexpr int kNumParams = (int) (sizeof (kParams) / sizeof (kParams[0]));

inline const juce::StringArray& sceneNames()
{
    static const juce::StringArray n { "Liquid Image Morph", "Fluid Distortion", "Fractal Tunnel", "Wireframe Geometry",
        "Procedural Plasma", "Nebula & Particles", "Radial Spectrum", "Typography & Forms", "Digital Feedback",
        "Kaleidoscope", "Glitch & RGB Split", "Waveform Scope", "Melting Image", "Recursive Fractal", "3D Forms" };
    return n;
}
inline const juce::StringArray& paletteNames()
{
    static const juce::StringArray n { "Black & White", "Red & Black", "Blue & Purple", "Pink & Cyan", "Acid Green",
        "Orange & Red", "Deep Blue", "Silver Grayscale", "Rainbow", "VHS", "Neon", "Mono + Accent" };
    return n;
}
inline const juce::StringArray& qualityNames() { static const juce::StringArray n { "Low", "Medium", "High", "Ultra" }; return n; }
inline const juce::StringArray& fpsNames()     { static const juce::StringArray n { "30 FPS", "60 FPS", "120 FPS" }; return n; }
inline constexpr float kQualityScale[4] = { .5f, .75f, 1.f, 1.5f };
inline constexpr float kFpsValues[3]    = { 30.f, 60.f, 120.f };

struct PaletteDef { float c[9]; bool rainbow; };   // shadow, mid, highlight (RGB)
inline constexpr PaletteDef kPalettes[12] =
{
    { { 0,0,0,      .5f,.5f,.5f,   1,1,1 },          false },  // Black & White
    { { 0,0,0,      .6f,0,.02f,    1,.15f,.1f },     false },  // Red & Black
    { { .02f,0,.15f,.2f,.1f,.8f,   .75f,.3f,1 },     false },  // Blue & Purple
    { { .1f,0,.15f, 1,.2f,.7f,     .2f,1,1 },        false },  // Pink & Cyan
    { { 0,.05f,0,   .2f,.9f,0,     .85f,1,.2f },     false },  // Acid Green
    { { .1f,0,0,    1,.3f,0,       1,.8f,.1f },      false },  // Orange & Red
    { { 0,0,.05f,   0,.15f,.6f,    .3f,.7f,1 },      false },  // Deep Blue
    { { .02f,.02f,.02f,.5f,.52f,.55f,.95f,.96f,1 },  false },  // Silver Grayscale
    { { .1f,0,.2f,  .5f,.2f,.9f,   1,1,1 },          true  },  // Rainbow (hue-cycled in shader)
    { { .05f,0,.1f, .9f,.2f,.5f,   .2f,.9f,.95f },   false },  // VHS
    { { 0,0,.05f,   1,0,.8f,       0,1,.6f },        false },  // Neon
    { { 0,0,0,      .3f,.3f,.3f,   1,.1f,.2f },      false },  // Mono + Accent
};

struct FactoryPreset { const char* name; int scene; int palette; std::vector<std::pair<const char*, float>> v; };
inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> p = {
        { "Liquid Chrome",     0, 7,  { {"distort", 1.1f}, {"feedback", .55f}, {"rgb", .35f}, {"glow", .8f} } },
        { "Underground Red",   0, 1,  { {"extreme", .7f}, {"grain", .35f}, {"vignette", .7f}, {"scanlines", .3f}, {"feedback", .6f} } },
        { "Acid Tunnel",       2, 4,  { {"speed", 1.4f}, {"glow", 1.2f}, {"symmetry", 8.f}, {"bass", 1.4f} } },
        { "Neon Spectrum",     6, 10, { {"glow", 1.4f}, {"feedback", .7f}, {"bass", 1.3f} } },
        { "Glitch VHS",        10, 9, { {"extreme", .8f}, {"rgb", .6f}, {"scanlines", .6f}, {"grain", .4f}, {"pixel", .1f} } },
        { "Kaleido Dream",     9, 3,  { {"symmetry", 10.f}, {"hue", .1f}, {"cycle", .3f}, {"glow", .9f} } },
        { "Meltdown",          12, 5, { {"extreme", .8f}, {"distort", 1.4f}, {"feedback", .5f} } },
        { "Deep Space",        5, 6,  { {"particles", .8f}, {"glow", 1.1f}, {"speed", .6f}, {"vignette", .6f} } },
        { "Wire Grid",         3, 0,  { {"complexity", .7f}, {"glow", 1.f}, {"feedback", .45f} } },
        { "Scope Pulse",       11, 4, { {"glow", 1.5f}, {"feedback", .75f}, {"scanlines", .25f} } },
        { "Fractal Bloom",     13, 2, { {"complexity", .8f}, {"cycle", .4f}, {"glow", 1.f} } },
        { "Molten Forms",      14, 5, { {"extreme", .6f}, {"glow", .9f}, {"grain", .2f} } },
    };
    return p;
}
} // namespace monie
