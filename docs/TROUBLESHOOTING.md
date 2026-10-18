# Troubleshooting

**Build: "Could not find git" / FetchContent fails** - install Git, check internet/proxy. Offline? Clone JUCE 8.0.6 and stb yourself and pass `-DFETCHCONTENT_SOURCE_DIR_JUCE=<path> -DFETCHCONTENT_SOURCE_DIR_STB=<path>`.

**Build: compile errors** - this code has never been compiled (see README). Paste the first error; most will be one-line JUCE API fixes. Make sure you use VS 2022 and JUCE 8.x (the UI uses `juce::FontOptions`).

**FL Studio doesn't list the plugin** - confirm `C:\Program Files\Common Files\VST3\monie visuals.vst3\Contents\x86_64-win\monie visuals.vst3` exists. Options > Manage plugins > add that folder to the scan paths > Find plugins. Use *Verify plugins* if it's marked failed.

**Plugin fails to load / "missing DLL"** - install the Microsoft Visual C++ Redistributable 2015-2022 (x64). Release builds use the dynamic CRT. Check with `dumpbin /dependents "monie visuals.vst3"` (VS command prompt).

**Preview black + orange text** - shader compile/link error: update your GPU driver; send me the text. The renderer is OpenGL compat + GLSL 1.20; very locked-down remote desktops/VMs only expose software OpenGL 1.1 and will not work.

**Images appear upside-down** - delete the line `t.y = 1.0 - t.y;` in `fetchImg` (Shaders/scene.frag) and rebuild.

**Image won't import** - WebP/TIFF are unsupported (convert to PNG/JPG). Files over 100 megapixels are refused. Red "MISSING" tile = file moved: select it and press Replace to relink.

**Low FPS / FL stutters** - lower Render Quality (Customize), set FPS to 30, reduce Glow/Feedback, avoid scene 15 (raymarch) and Fluid on weak GPUs. Rendering is on its own thread; the audio thread never touches the GPU, so audio should not glitch from visuals - if it does, tell me.

**Fullscreen inside FL** - the plugin enters preview-only mode inside its own window; maximize FL's plugin window yourself, or use the Standalone for true fullscreen. Esc/F11 exits.

**Visuals don't react** - the plugin must sit on an insert that carries audio (e.g. Master). Check the Audio tab meters. If bass/mid/treble stay at 0, the insert is silent.
