// Data-driven scene catalogue: categories, tags, search, favorites. Adding a scene = one entry here + one GLSL function
// (see docs/ADDING_SCENES.md). The `index` is the value of the "scene" parameter / u_scene uniform. No JUCE dependency.
#pragma once
#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include <string>
#include <vector>
namespace monie
{
struct SceneInfo { int index; std::string name, category, tags; };

inline const std::vector<SceneInfo>& sceneCatalogue()
{
    static const std::vector<SceneInfo> s = {
        { 0,  "Liquid Image Morph", "Image-Based",            "image liquid warp morph portrait" },
        { 1,  "Fluid Distortion",   "Abstract & Experimental","fluid noise organic flow" },
        { 2,  "Fractal Tunnel",     "Abstract & Experimental","tunnel fractal zoom infinite" },
        { 3,  "Wireframe Geometry", "Geometry & 3D",          "wireframe grid polygon retro" },
        { 4,  "Procedural Plasma",  "Abstract & Experimental","plasma psychedelic" },
        { 5,  "Nebula & Particles", "Nature & Atmosphere",    "nebula stars particles space dust" },
        { 6,  "Radial Spectrum",    "Music-Reactive",         "spectrum radial bars frequency" },
        { 7,  "Typography & Forms", "Music-Reactive",         "text letters geometry squares" },
        { 8,  "Digital Feedback",   "Underground",            "feedback video trails recursion" },
        { 9,  "Kaleidoscope",       "Geometry & 3D",          "kaleidoscope mirror symmetry image" },
        { 10, "Glitch & RGB Split", "Underground",            "glitch rgb vhs datamosh image" },
        { 11, "Waveform Scope",     "Music-Reactive",         "oscilloscope waveform lines" },
        { 12, "Melting Image",      "Image-Based",            "melt drip image liquid" },
        { 13, "Recursive Fractal",  "Abstract & Experimental","fractal kifs recursion" },
        { 14, "3D Forms",           "Geometry & 3D",          "raymarch sphere torus 3d" },
    };
    return s;
}
inline std::string lower (std::string s) { for (auto& c : s) c = (char) std::tolower ((unsigned char) c); return s; }
inline std::vector<std::string> sceneCategories()
{ std::vector<std::string> c; for (auto& s : sceneCatalogue()) if (std::find (c.begin(), c.end(), s.category) == c.end()) c.push_back (s.category); return c; }

// every whitespace-separated word of the query must appear in name, category or tags; empty category = all
inline std::vector<int> searchScenes (const std::string& query, const std::string& category = "", const std::set<int>* favoritesOnly = nullptr)
{
    std::vector<std::string> words; { std::istringstream q (lower (query)); std::string w; while (q >> w) words.push_back (w); }
    std::vector<int> out;
    for (auto& s : sceneCatalogue())
    {
        if (! category.empty() && s.category != category) continue;
        if (favoritesOnly && ! favoritesOnly->count (s.index)) continue;
        const std::string hay = lower (s.name + " " + s.category + " " + s.tags); bool ok = true;
        for (auto& w : words) if (hay.find (w) == std::string::npos) { ok = false; break; }
        if (ok) out.push_back (s.index);
    }
    return out;
}
inline std::string favoritesToText (const std::set<int>& f) { std::string o; for (int i : f) { if (! o.empty()) o += ','; o += std::to_string (i); } return o; }
inline std::set<int> favoritesFromText (const std::string& t)
{ std::set<int> f; std::istringstream s (t); std::string c; while (std::getline (s, c, ',')) { if (c.empty() || c.find_first_not_of ("0123456789") != std::string::npos) continue; int v = std::atoi (c.c_str()); if (v < (int) sceneCatalogue().size()) f.insert (v); } return f; }
}
