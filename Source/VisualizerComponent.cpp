#include "VisualizerComponent.h"
#include <BinaryData.h>
#include <cstring>

using namespace juce::gl;
using namespace monie;

// ---------------- shader program helper ----------------
bool VisualizerComponent::Prog::build (juce::OpenGLContext& c, const juce::String& vs, const juce::String& fs, juce::String& err)
{
    loc.clear();
    p = std::make_unique<juce::OpenGLShaderProgram> (c);
    if (p->addVertexShader (vs) && p->addFragmentShader (fs) && p->link())
    {
        pos = glGetAttribLocation (p->getProgramID(), "aPos");
        return pos >= 0;
    }
    err = p->getLastError();
    p.reset();
    return false;
}
int VisualizerComponent::Prog::L (const std::string& n)
{
    auto it = loc.find (n);
    if (it != loc.end()) return it->second;
    const int l = glGetUniformLocation (p->getProgramID(), n.c_str());
    loc.emplace (n, l);
    return l;
}
void VisualizerComponent::Prog::f1 (const std::string& n, float v) { const int l = L (n); if (l >= 0) glUniform1f (l, v); }
void VisualizerComponent::Prog::f2 (const std::string& n, float a, float b) { const int l = L (n); if (l >= 0) glUniform2f (l, a, b); }
void VisualizerComponent::Prog::f3 (const std::string& n, float a, float b, float c) { const int l = L (n); if (l >= 0) glUniform3f (l, a, b, c); }
void VisualizerComponent::Prog::i1 (const std::string& n, int v) { const int l = L (n); if (l >= 0) glUniform1i (l, v); }

// ---------------- component ----------------
VisualizerComponent::VisualizerComponent (MonieProcessor& p) : proc (p)
{
    for (int i = 0; i < kNumParams; ++i)
    {
        raw[i] = proc.apvts.getRawParameterValue (kParams[i].id);
        uniformNames.push_back (std::string ("u_") + kParams[i].id);
    }
    pScene = proc.apvts.getRawParameterValue ("scene");
    pPalette = proc.apvts.getRawParameterValue ("palette");
    pQuality = proc.apvts.getRawParameterValue ("quality");
    pFps = proc.apvts.getRawParameterValue ("fps");

    setOpaque (true);
    ctx.setRenderer (this);
    ctx.setComponentPaintingEnabled (false);
    ctx.setContinuousRepainting (true);
    ctx.setSwapInterval (1);           // vsync; the 30 FPS cap is done by skipping the heavy pass (see renderOpenGL)
    ctx.attachTo (*this);
}

VisualizerComponent::~VisualizerComponent() { ctx.detach(); }

void VisualizerComponent::setMainImage (const juce::Image& img)
{
    const juce::ScopedLock sl (imgLock);
    pendingImage = img; pendingDirty = true;
}

juce::String VisualizerComponent::getError() const { const juce::ScopedLock sl (errLock); return errorText; }
void VisualizerComponent::setError (const juce::String& e) { const juce::ScopedLock sl (errLock); errorText = e; }

void VisualizerComponent::newOpenGLContextCreated()
{
    juce::String err;
    const juce::String vs (BinaryData::pass_vert, BinaryData::pass_vertSize);
    bool ok = scene.build (ctx, vs, juce::String (BinaryData::scene_frag, BinaryData::scene_fragSize), err);
    if (! ok) setError ("Scene shader failed:\n" + err);
    else
    {
        ok = post.build (ctx, vs, juce::String (BinaryData::post_frag, BinaryData::post_fragSize), err);
        if (! ok) setError ("Post shader failed:\n" + err);
    }
    gpuReady = ok;

    const float quad[8] = { -1, -1, 1, -1, -1, 1, 1, 1 };
    glGenBuffers (1, &vbo);
    glBindBuffer (GL_ARRAY_BUFFER, vbo);
    glBufferData (GL_ARRAY_BUFFER, sizeof (quad), quad, GL_STATIC_DRAW);
    glBindBuffer (GL_ARRAY_BUFFER, 0);

    glGenTextures (1, &specTex);
    glBindTexture (GL_TEXTURE_2D, specTex);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 64, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, specBytes);
    glBindTexture (GL_TEXTURE_2D, 0);

    lastTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
    // image may have been set before the context existed
    const juce::ScopedLock sl (imgLock);
    if (pendingImage.isValid()) pendingDirty = true;
}

void VisualizerComponent::openGLContextClosing()
{
    gpuReady = false;
    scene.p.reset(); post.p.reset();
    fb[0].release(); fb[1].release();
    tex[0].release(); tex[1].release();
    texValid[0] = texValid[1] = false;
    if (vbo != 0) { glDeleteBuffers (1, &vbo); vbo = 0; }
    if (specTex != 0) { glDeleteTextures (1, &specTex); specTex = 0; }
}

void VisualizerComponent::uploadPendingImage()
{
    juce::Image img;
    {
        const juce::ScopedLock sl (imgLock);
        if (! pendingDirty) return;
        img = pendingImage; pendingDirty = false;
    }
    if (! img.isValid()) { texValid[0] = texValid[1] = false; return; }
    const int next = 1 - texA;
    tex[next].loadImage (img);               // texture upload happens here, on the GL thread
    texAspect[next] = (float) img.getWidth() / (float) img.getHeight();
    const bool hadPrev = texValid[texA];
    texValid[next] = true;
    texA = next;
    imgBlend = hadPrev ? 1.f : 0.f;          // crossfade old -> new at "Image Transition Speed"
}

void VisualizerComponent::drawQuad (Prog& pr)
{
    glBindBuffer (GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray ((GLuint) pr.pos);
    glVertexAttribPointer ((GLuint) pr.pos, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray ((GLuint) pr.pos);
    glBindBuffer (GL_ARRAY_BUFFER, 0);
}

void VisualizerComponent::setCommon (Prog& pr, float t)
{
    for (int i = 0; i < kNumParams; ++i) pr.f1 (uniformNames[(size_t) i], raw[i]->load());
    const int pi = juce::jlimit (0, 11, (int) pPalette->load());
    const auto& pd = kPalettes[pi];
    pr.f3 ("u_c0", pd.c[0], pd.c[1], pd.c[2]);
    pr.f3 ("u_c1", pd.c[3], pd.c[4], pd.c[5]);
    pr.f3 ("u_c2", pd.c[6], pd.c[7], pd.c[8]);
    pr.f1 ("u_rainbow", pd.rainbow ? 1.f : 0.f);
    pr.f1 ("u_time", t);
    pr.f1 ("a_bass", bassE); pr.f1 ("a_mid", midE); pr.f1 ("a_treb", trebE);
    pr.f1 ("a_rms", rmsE); pr.f1 ("a_flux", fluxE);
}

void VisualizerComponent::renderOpenGL()
{
    jassert (juce::OpenGLHelpers::isContextActive());
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const float dt = (float) juce::jlimit (0.0005, 0.1, now - lastTime);
    lastTime = now;

    const float scaleF = (float) ctx.getRenderingScale();
    const int pw = juce::jmax (16, juce::roundToInt (getWidth() * scaleF));
    const int ph = juce::jmax (16, juce::roundToInt (getHeight() * scaleF));

    glDisable (GL_BLEND); glDisable (GL_DEPTH_TEST); glDisable (GL_CULL_FACE);

    if (! gpuReady.load() || scene.p == nullptr || post.p == nullptr)
    {   // graceful fallback: dark frame; the editor shows the shader error text
        glViewport (0, 0, pw, ph);
        juce::OpenGLHelpers::clear (juce::Colour (0xff0b0b10));
        return;
    }

    auto P = [this] (const char* id) -> float
    {
        for (int i = 0; i < kNumParams; ++i) if (std::strcmp (kParams[i].id, id) == 0) return raw[i]->load();
        return 0.f;
    };

    // ---------- audio-reactive control signals (CPU side, smoothed) ----------
    const float master = P ("master"), drive = 0.25f + master * 1.5f;
    const float sm = 1.f - 0.8f * P ("smooth");
    const float atkRate = juce::jmap (P ("attack"), 3.f, 80.f) * sm, relRate = juce::jmap (P ("release"), 30.f, 1.5f) * sm;
    auto env = [&] (float& e, float target) { const float r = target > e ? atkRate : relRate; e += (target - e) * (1.f - std::exp (-dt * r)); };
    auto& A = proc.analysis;
    env (bassE, juce::jmin (1.6f, A.bass.load() * P ("bass") * drive));
    env (midE,  juce::jmin (1.6f, A.mid.load()  * P ("mid")  * drive));
    env (trebE, juce::jmin (1.6f, A.treble.load() * P ("treble") * drive));
    env (rmsE,  juce::jmin (1.6f, A.rms.load() * 2.f * drive));
    env (fluxE, juce::jmin (1.6f, A.flux.load() * drive));
    const uint32_t bc = A.beatCount.load();
    if (bc != lastBeat) { lastBeat = bc; beatEnv = 1.f; }
    beatEnv *= std::exp (-dt * 5.f);
    const float beatOut = juce::jmin (1.6f, beatEnv * P ("beat") * drive);

    for (int i = 0; i < AnalysisFrame::numBands; ++i)
    {
        const float sens = i < 16 ? P ("bass") : (i < 40 ? P ("mid") : P ("treble"));
        env (bandE[i], juce::jmin (1.f, A.bands[i].load() * sens * drive));
        specBytes[i * 4] = (unsigned char) juce::jlimit (0, 255, (int) (bandE[i] * 255.f));
        specBytes[i * 4 + 3] = 255;
        specBytes[(64 + i) * 4] = (unsigned char) juce::jlimit (0, 255, (int) (A.wave[i].load() * 255.f));
        specBytes[(64 + i) * 4 + 3] = 255;
    }
    glActiveTexture (GL_TEXTURE3);
    glBindTexture (GL_TEXTURE_2D, specTex);
    glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 64, 2, GL_RGBA, GL_UNSIGNED_BYTE, specBytes);

    time += dt * P ("speed");
    const int sceneIdx = juce::jlimit (0, 14, (int) pScene->load());
    if (lastScene >= 0 && sceneIdx != lastScene) sceneFade = 1.f;     // crossfade from last frame of previous scene
    lastScene = sceneIdx;
    sceneFade = juce::jmax (0.f, sceneFade - dt / juce::jmax (0.05f, P ("xfade")));
    imgBlend = juce::jmax (0.f, imgBlend - dt * P ("imgspeed"));

    uploadPendingImage();

    // ---------- render targets (resolution scaling by quality) ----------
    const float qs = kQualityScale[juce::jlimit (0, 3, (int) pQuality->load())];
    const int fw = juce::jlimit (16, 4096, juce::roundToInt (pw * qs)), fh = juce::jlimit (16, 4096, juce::roundToInt (ph * qs));
    for (auto& f : fb)
        if (! f.isValid() || f.getWidth() != fw || f.getHeight() != fh)
        {
            f.release();
            if (f.initialise (ctx, fw, fh))
            {
                f.makeCurrentRenderingTarget();
                glClearColor (0, 0, 0, 1); glClear (GL_COLOR_BUFFER_BIT);
                f.releaseAsRenderingTarget();
            }
        }
    if (! fb[0].isValid() || ! fb[1].isValid()) { glViewport (0, 0, pw, ph); juce::OpenGLHelpers::clear (juce::Colours::black); return; }

    // ---------- frame-rate cap: only run the heavy scene pass when due ----------
    const float targetFps = kFpsValues[juce::jlimit (0, 2, (int) pFps->load())];
    const bool due = (now - lastSceneTime) >= (1.0 / targetFps) - 0.002;
    if (due)
    {
        const double frameDt = now - lastSceneTime; lastSceneTime = now;
        if (frameDt > 0.0) fps = fps.load() * 0.9f + (float) (1.0 / frameDt) * 0.1f;

        const int w = writeIdx, r = 1 - w;
        fb[w].makeCurrentRenderingTarget();
        glViewport (0, 0, fw, fh);
        scene.p->use();
        glActiveTexture (GL_TEXTURE0); glBindTexture (GL_TEXTURE_2D, fb[r].getTextureID());
        glActiveTexture (GL_TEXTURE1); glBindTexture (GL_TEXTURE_2D, texValid[texA] ? tex[texA].getTextureID() : 0);
        glActiveTexture (GL_TEXTURE2); glBindTexture (GL_TEXTURE_2D, texValid[1 - texA] ? tex[1 - texA].getTextureID() : (texValid[texA] ? tex[texA].getTextureID() : 0));
        glActiveTexture (GL_TEXTURE3); glBindTexture (GL_TEXTURE_2D, specTex);
        scene.i1 ("u_prev", 0); scene.i1 ("u_imgA", 1); scene.i1 ("u_imgB", 2); scene.i1 ("u_spec", 3);
        scene.i1 ("u_scene", sceneIdx);
        scene.f2 ("u_res", (float) fw, (float) fh);
        scene.f1 ("u_hasImg", texValid[texA] ? 1.f : 0.f);
        scene.f1 ("u_blend", texValid[1 - texA] ? imgBlend : 0.f);
        scene.f1 ("u_aspA", texAspect[texA]); scene.f1 ("u_aspB", texAspect[1 - texA]);
        scene.f1 ("u_fade", sceneFade);
        setCommon (scene, time);
        scene.f1 ("a_beat", beatOut);
        drawQuad (scene);
        fb[w].releaseAsRenderingTarget();
        lastRead = w; writeIdx = r;
    }

    // ---------- post pass to the window ----------
    glViewport (0, 0, pw, ph);
    post.p->use();
    glActiveTexture (GL_TEXTURE0); glBindTexture (GL_TEXTURE_2D, fb[lastRead].getTextureID());
    post.i1 ("u_tex", 0);
    post.f2 ("u_res", (float) pw, (float) ph);
    setCommon (post, time);
    post.f1 ("a_beat", beatOut);
    drawQuad (post);
    glActiveTexture (GL_TEXTURE0);
}
