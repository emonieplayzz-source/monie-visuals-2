# monie visuals  (v0.2.0-dev)

> **2.0 upgrade in progress - see `docs/V2_STATUS.md`.** This pass added tested logic engines (timeline/keyframes/undo, lyrics LRC/SRT, audio-mapping, randomizer, scene catalogue) and extra analysis bands. They are not wired to the UI yet. Typography, video, effect stack, layers, export and the standalone app are not started.


An original GPU audio-reactive visualizer for **Windows 11 x64 / FL Studio 26** (VST3 + Standalone), built with C++17, JUCE 8 and OpenGL/GLSL.
Drop images onto the preview and they morph, melt, warp and glitch with your music. 15 original shader scenes, 12 palettes, 42 automatable parameters.

> ## READ THIS FIRST - honest status
> This source tree was written in a Linux sandbox with **no Windows toolchain, no JUCE, and no network access**.
> **The plugin has NOT been compiled, loaded in FL Studio, or visually tested.** There is no binary in this package, and none is claimed.
> What *was* verified: everything in `Source/Core/` compiled with g++ and passes `tests/test_analyzer.cpp` (14 checks) and `tests/test_engines.cpp` (52 checks). Everything else is unverified source code
> and **will probably need a few compile-error fixes on first build** (JUCE API typos, GLSL driver quirks). Send me the first error output and I'll fix it.

## What's implemented (in source) vs. not

| Area | Status |
|---|---|
| VST3 + Standalone, stereo/mono, transparent passthrough (buffer is never modified) | Written, unbuilt |
| Audio thread: copy to preallocated lock-free FIFO only (no alloc/locks/IO/GPU) | Written, unbuilt |
| Analysis thread: Hann FFT, bass/mid/treble, RMS, peak, spectral flux, beat detect, per-band AGC, any sample rate / block size | **Tested (unit test)** |
| Attack / release / smoothing / per-band sensitivities | Written, unbuilt |
| OpenGL renderer: ping-pong feedback FBOs, quality scaling (50-150%), FPS cap 30/60/120, shader error overlay, GPU cleanup | Written, unbuilt |
| 15 scenes (liquid image morph, fluid, tunnel, wireframe, plasma, nebula, radial spectrum, typography, feedback, kaleidoscope, glitch, scope, melting image, recursive fractal, 3D forms) | Written, unbuilt |
| Image drag & drop from Explorer, Import/Remove/Replace/Reorder, thumbnails, main image, crossfade between images, missing-file detection + relink (Replace) | Written, unbuilt |
| Formats: **PNG, JPG/JPEG, BMP, GIF, TGA** (via stb_image) | Written, unbuilt |
| **WebP, TIFF: NOT supported** (need libwebp/libtiff; the UI tells you clearly) | Not implemented |
| Morph/effects: liquid warp, wave, fluid, RGB split, chromatic aberration, feedback trails, swirl/kaleido, mirror, zoom/rotate, noise displace, melt, glitch, edge, posterize, invert, threshold, pixelate, glow/bloom, grain, vignette, scanlines | Written, unbuilt |
| Pixel sorting | Not implemented (approximated by glitch row-shift) |
| Color workspace: hue, sat, brightness, contrast, gamma, temperature, tint, cycling, audio color reactivity, gradient map, background level, 12 palettes incl. rainbow | Written, unbuilt |
| **Custom palette editor / gradient editor / hex & HSV pickers / saved user palettes** | **Not implemented** (palette = the 12 built-ins + hue/sat/etc.) |
| **Draggable/reorderable effect stack** | **Not implemented** (fixed order; each effect has strength, 0 = off) |
| **Per-effect enable toggles** | Not implemented (set the amount to 0) |
| Customize workspace: all 42 parameters, quality, FPS; double-click any slider to reset it; Reset All | Written, unbuilt |
| All params exposed to FL automation (scene, palette, quality, fps + 42 sliders) | Written, unbuilt |
| Presets: 12 factory (in code) + user presets (save / load / duplicate / delete) in `Documents\monie visuals\Presets\*.mvpreset`, include image paths | Written, unbuilt |
| Project/session recall (state + images saved in the FL project) | Written, unbuilt |
| Fullscreen: preview-only mode, Esc/F11 to exit (true kiosk mode in Standalone only) | Written, unbuilt |
| Themes: Neon / Grayscale / High Contrast | Written, unbuilt |
| Scene crossfade (time adjustable), random scene, scene "thumbnails" (procedural swatches, not live renders) | Written, unbuilt |
| MIDI assignment | Not implemented (scene is an automatable parameter - link a controller in FL) |
| **MP4 recording / export** | **Not implemented.** Record with OBS (see below). Planned: standalone + FFmpeg pipe |

## Build on Windows 11 (about 15 minutes, mostly downloading)
1. Install **Visual Studio 2022** (workload "Desktop development with C++"), **CMake** (3.22+), and **Git**.
2. Open this folder in a terminal and run: `scripts\build_windows.bat`
   (First configure downloads JUCE 8.0.6 and stb_image from GitHub, so you need internet.)
3. Outputs: `build\MonieVisuals_artefacts\Release\VST3\monie visuals.vst3` and `...\Standalone\monie visuals.exe`.
   The script also runs the analysis unit test.
4. Optional: `scripts\package_release.bat` creates a `release\` folder.

**No Visual Studio? ** Push this folder to a GitHub repo; `.github/workflows/build-windows.yml` builds on a Windows runner and gives you a downloadable artifact (Actions tab).

## Install in FL Studio 26
1. Run `scripts\install_vst3.bat` **as Administrator** (copies to `C:\Program Files\Common Files\VST3\monie visuals.vst3`).
2. FL Studio: **Options > Manage plugins**, click **Find plugins** (make sure the VST3 folder is in the scan list).
3. Add it to a mixer insert slot (it's an effect, not an instrument): Plugin database > Installed > Effects > *monie visuals*. Put it on the Master or a bus you want to visualize.

## Verify it (please actually do these)
1. **Loads:** the plugin window opens with the dark UI and an animated preview. If the preview is black and shows orange text, that's a shader error - copy the text to me.
2. **Audio passthrough (null test):** Put two copies of the same loop on two mixer tracks routed to Master; put *monie visuals* on one, flip the polarity of one track (the Ø button next to the pan knob) - output should be silence. Or compare an Edison recording with and without the plugin.
3. **Reactivity:** play a kick loop; the Audio tab meters should jump and BEAT should flash. Raise Bass Sensitivity.
4. **Images:** drag a PNG from Explorer onto the preview (try `assets\examples\`). Select scene 1 "Liquid Image Morph".
5. **Automation:** right-click a slider > *Create automation clip* (FL shows plugin params in the plugin wrapper menu). Move the clip; the visuals should follow. Also test Scene (choice) automation.
6. **Project recall:** save the FL project, reopen it; scene, palette and images should come back. Rename an image file to test the red MISSING tile + Replace.
7. **Resize / fullscreen:** drag the window corner; press Fullscreen, then Esc.

## Recording for YouTube (until built-in export exists)
Open the plugin window (or the Standalone), press **Fullscreen**, and capture with **OBS Studio** (Window Capture or Display Capture; encoder NVENC/x264; MP4). Add the DAW's audio through OBS *Desktop Audio*. For frame-exact sync you can render the audio from FL to WAV and mux with FFmpeg later:
`ffmpeg -i capture.mp4 -i song.wav -c:v copy -c:a aac -shortest out.mp4`

## Defaults I chose (you weren't asked)
OpenGL 2.1/compat contexts with GLSL 1.20 shaders (maximum driver compatibility; `RenderBackend` abstraction is not yet split out - renderer lives in `VisualizerComponent`). Own radix-2 FFT instead of `juce::dsp::FFT` so the analysis core is unit-testable without JUCE. Max image size is downscaled to 2048 px on load. Beat detector reacts to any strong onset (hats too), tune with *Beat Sensitivity*.

## Layout
```
Source/Core/AudioAnalyzer.h    FFT, bands, flux, beats (no JUCE; unit tested)
Source/PluginProcessor.*       passthrough, FIFO, analysis thread, parameters, presets, state
Source/VisualizerComponent.*   OpenGL renderer
Source/PluginEditor.*          UI (workspaces, scene list, image library, presets, meters)
Source/ImageLibrary.*          async image decode (stb), library model
Source/Params.h                single table of parameters, palettes, factory presets
Shaders/*.frag|vert            all shaders (embedded at build time)
tests/test_analyzer.cpp        analysis unit test
```
See `docs/TROUBLESHOOTING.md` and `docs/LICENSES.md`.

## Roadmap (in priority order)
1. First successful Windows build + fixes. 2. Color workspace: custom gradient/palette editor, hex/HSV pickers, saved palettes. 3. Effect-stack reorder + per-effect toggles.
4. Standalone renderer with FFmpeg MP4 export (1080p/4K). 5. WebP/TIFF import. 6. Real scene thumbnails, MIDI learn.
