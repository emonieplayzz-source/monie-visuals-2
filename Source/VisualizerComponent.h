#pragma once
#include <juce_opengl/juce_opengl.h>
#include <unordered_map>
#include "PluginProcessor.h"

// GPU renderer. All GL work happens on JUCE's OpenGL thread, never on the audio thread or the message thread.
class VisualizerComponent : public juce::Component, public juce::OpenGLRenderer
{
public:
    explicit VisualizerComponent (MonieProcessor&);
    ~VisualizerComponent() override;

    void setMainImage (const juce::Image&);          // message thread; uploaded lazily on the GL thread
    juce::String getError() const;                   // shader compile/link problems, empty if fine
    float getFps() const { return fps.load(); }
    bool  isGpuReady() const { return gpuReady.load(); }

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;
    void paint (juce::Graphics&) override {}

private:
    struct Prog
    {
        std::unique_ptr<juce::OpenGLShaderProgram> p;
        std::unordered_map<std::string, int> loc;
        int pos = -1;
        bool build (juce::OpenGLContext&, const juce::String& vs, const juce::String& fs, juce::String& err);
        int  L (const std::string& n);
        void f1 (const std::string& n, float v);
        void f2 (const std::string& n, float a, float b);
        void f3 (const std::string& n, float a, float b, float c);
        void i1 (const std::string& n, int v);
    };
    void drawQuad (Prog&);
    void setCommon (Prog&, float time);
    void uploadPendingImage();
    void setError (const juce::String&);

    MonieProcessor& proc;
    juce::OpenGLContext ctx;
    Prog scene, post;
    juce::OpenGLFrameBuffer fb[2];
    juce::OpenGLTexture tex[2];
    bool texValid[2] = { false, false };
    float texAspect[2] = { 1.f, 1.f };
    int  texA = 0;                       // which of tex[] currently holds "A"
    unsigned vbo = 0, specTex = 0;

    std::atomic<float>* raw[monie::kNumParams] {};
    std::atomic<float> *pScene, *pPalette, *pQuality, *pFps;
    std::vector<std::string> uniformNames;

    juce::CriticalSection imgLock;
    juce::Image pendingImage; bool pendingDirty = false;

    // render-thread state
    double lastTime = 0, lastSceneTime = 0;
    float time = 0, bassE = 0, midE = 0, trebE = 0, rmsE = 0, fluxE = 0, beatEnv = 0, imgBlend = 0, sceneFade = 0;
    float bandE[monie::AnalysisFrame::numBands] {};
    uint32_t lastBeat = 0;
    int lastScene = -1, writeIdx = 0, lastRead = 0;
    unsigned char specBytes[64 * 2 * 4] {};

    std::atomic<float> fps { 0 };
    std::atomic<bool> gpuReady { false };
    mutable juce::CriticalSection errLock;
    juce::String errorText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VisualizerComponent)
};
