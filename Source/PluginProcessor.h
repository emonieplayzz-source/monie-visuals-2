#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Core/AudioAnalyzer.h"
#include "ImageLibrary.h"
#include "Params.h"

class MonieProcessor : public juce::AudioProcessor
{
public:
    MonieProcessor();
    ~MonieProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "monie visuals"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- shared state ----
    juce::AudioProcessorValueTreeState apvts;
    monie::ImageLibrary library;   // message thread only

    // Written by the analysis thread, read by the render thread. Plain atomics: no locks anywhere.
    struct Analysis
    {
        std::atomic<float> bass { 0 }, mid { 0 }, treble { 0 }, rms { 0 }, peak { 0 }, flux { 0 }, lowMid { 0 }, highMid { 0 }, transient { 0 };
        std::atomic<uint32_t> beatCount { 0 };
        std::atomic<float> bands[monie::AnalysisFrame::numBands];
        std::atomic<float> wave[monie::AnalysisFrame::numWave];
        Analysis() { for (auto& b : bands) b = 0; for (auto& w : wave) w = 0.5f; }
    } analysis;

    // ---- presets ----
    void resetAll();
    void setParam (const juce::String& id, float plainValue);
    void applyFactoryPreset (int index);
    void varyParams (unsigned seed, float amount);   // reproducible controlled variation of all sliders
    static juce::File userPresetDir();
    juce::Array<juce::File> getUserPresets() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File&);
    bool deleteUserPreset (const juce::File&);
    bool duplicateUserPreset (const juce::File&);

    int  getTheme() const { return (int) apvts.state.getProperty ("theme", 0); }
    void setTheme (int t) { apvts.state.setProperty ("theme", t, nullptr); }

private:
    class AnalysisThread;
    std::unique_ptr<juce::XmlElement> createStateXml();
    void restoreStateXml (const juce::XmlElement&);
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void startAnalysis (double sr);
    void stopAnalysis();

    static constexpr int fifoSize = 1 << 15;
    juce::AbstractFifo fifo { fifoSize };
    std::vector<float> fifoData;                 // preallocated in constructor
    std::array<float, 1024> scratch {};          // preallocated, audio-thread mono mix chunk
    std::unique_ptr<AnalysisThread> analysisThread;
    std::atomic<float>* beatSensParam = nullptr;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonieProcessor)
};
