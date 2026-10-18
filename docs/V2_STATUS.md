# Monie Visuals 2.0 - status (pass 1)

Legend: **TESTED** = compiled with g++ and passing unit tests in the Linux sandbox. **WRITTEN** = source exists, never compiled (no JUCE/Windows here). **NOT STARTED**.
Nothing has been built or run on Windows, and nothing has been loaded in FL Studio.

## Existing 1.0 code (inspected)
Works in design (unbuilt): passthrough VST3/standalone, lock-free FIFO + analysis thread, one OpenGL renderer with 15 fixed scenes sharing 42 global parameters, image drag/drop/library, 12 palettes, presets, simple tabbed UI.
Architectural limits found: single monolithic renderer (no layers/compositing graph), global params only (no per-layer/per-scene params), UI is one editor class, no project model, no media decoding beyond stb images, no text rendering, no timeline.

## This pass: new engines (plain C++, no JUCE, TESTED: 52 + 14 checks)
| Engine | File | What it does |
|---|---|---|
| Timeline core | `Core/Timeline.h` | 6 easing modes, keyframe tracks (evaluate, replace, copy/paste), clip move/trim(source in-point aware)/split/duplicate, snapping, undo/redo stack |
| Lyrics | `Core/Lyrics.h` | LRC (multi-timestamp, [offset], enhanced word tags), SRT (CRLF, tags stripped), plain text + even auto-distribution, SRT/LRC export, active line/word, lead-in, prev/next view, manual offset. Never fetches lyrics; no recognition |
| Audio mapping | `Core/AudioMapping.h` | 10 sources, sensitivity, input range, out min/max, 5 curves, threshold, invert, attack/release, smoothing, beat-quantize, seeded random, multi-source sum, text preset save/load |
| Analyzer | `Core/AudioAnalyzer.h` | added low-mid, high-mid, transient (published to atomics) |
| Randomizer | `Core/Randomizer.h` | seeded vary/randomize with locks |
| Scene catalogue | `Core/SceneRegistry.h` | categories, tags, multi-word search, favorites (+ text persistence) |

Wired into the plugin (WRITTEN, unbuilt): new analyzer bands in the processor; a **Vary** button (reproducible +/-15% variation of all sliders; toggles locked).
**Engines NOT yet wired to UI/renderer:** timeline, lyrics, mapping, favorites/search. They have no on-screen controls yet, so as of now you cannot use them from the plugin.

## Requirement-by-requirement
| # | Area | Status |
|---|---|---|
| 1 | UI redesign (toolbar, left/right panels, timeline, docking, undo UI, shortcuts, context menus, tooltips everywhere, 9 workspaces) | NOT STARTED (1.0 UI has 8 workspaces, themes, a few tooltips). Needs a component-per-panel rewrite |
| 2 | 60+ scenes | 15 exist; catalogue/search/favorites logic TESTED; thumbnails are procedural swatches; **no new scenes added this pass**, no transitions with blend modes/easing yet (only crossfade time) |
| 3 | Typography | NOT STARTED. Needs a text-to-texture/SDF pipeline in the renderer |
| 4 | Lyrics | Parsing/timing/export/display-query logic TESTED; no UI, no on-canvas rendering (depends on #3), no timeline editor. Forced alignment: not implemented (will be an optional external tool, e.g. whisper.cpp/aeneas, offline, never uploading by default) |
| 5 | Video | NOT STARTED. Requires FFmpeg libs (LGPL build recommended); decode on worker threads with a frame cache. Standalone-first |
| 6 | Timeline/keyframes | Data model + interpolation + clip editing + undo TESTED; no UI, not connected to parameters |
| 7 | Effect stack | NOT STARTED (1.0 has fixed-order global effects) |
| 8 | Color grading | 1.0 basics only (hue/sat/bright/contrast/gamma/temp/tint/gradient-map). Curves, wheels, LUT, custom gradients, before/after: NOT STARTED |
| 9 | Audio mapping | Engine TESTED; no mapping-editor UI, not yet driving renderer parameters; no live graph beyond the 1.0 meters |
| 10 | Layers/compositing/shapes | NOT STARTED |
| 11 | Project format/autosave/packaging | NOT STARTED beyond 1.0 state + .mvpreset files |
| 12 | Export | NOT STARTED (record via OBS, see README). Plan: standalone app, offline frame-by-frame render driven by a WAV file + FFmpeg pipe, so audio sync is exact; plugin will not claim access to the song file |
| 13 | Workflow tools | Seeded Vary (WRITTEN), randomizer w/ locks (TESTED engine), reset/double-click reset (1.0). A/B, copy/paste effects, perf monitor: NOT STARTED |
| 14 | Architecture | New logic lives in dependency-free `Source/Core/` so it is testable. Renderer abstraction + split into systems still to do |
| 16 | Deliverables | Updated source, CMake, scripts, tests, example lyric files, docs: yes. Example project, text-animation/effect presets, standalone video app, compiled build: **no** |

## Recommended next passes
1. **Get 1.0 + this pass to compile on Windows** (use the GitHub Actions workflow if you don't have Visual Studio) - everything else builds on that.
2. Layer model + compositor + per-layer parameter blocks (unblocks 7, 10, per-scene params, 2).
3. Text rendering (JUCE glyph -> texture atlas) + typography UI, then lyrics on canvas.
4. Mapping editor and timeline UI wired to the tested engines.
5. Standalone app with FFmpeg decode/export.
