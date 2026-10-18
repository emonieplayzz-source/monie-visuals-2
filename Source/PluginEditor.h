#pragma once
#include "PluginProcessor.h"
#include "VisualizerComponent.h"

namespace monie
{
struct MonieLookAndFeel : juce::LookAndFeel_V4
{
    juce::Colour accent { 0xff00e5ff }, bg { 0xff0b0b10 }, panel { 0xff14141c };
    MonieLookAndFeel() { setTheme (0); }
    void setTheme (int t)
    {
        if (t == 1)      { accent = juce::Colour (0xffd8d8d8); bg = juce::Colour (0xff0c0c0c); panel = juce::Colour (0xff161616); }
        else if (t == 2) { accent = juce::Colour (0xffffe600); bg = juce::Colours::black;      panel = juce::Colour (0xff0a0a0a); }
        else             { accent = juce::Colour (0xff00e5ff); bg = juce::Colour (0xff0b0b10); panel = juce::Colour (0xff14141c); }
        setColour (juce::ResizableWindow::backgroundColourId, bg);
        setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (.85f));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (.8f));
        setColour (juce::TextButton::buttonColourId, panel.brighter (.12f));
        setColour (juce::TextButton::buttonOnColourId, accent.withAlpha (.85f));
        setColour (juce::TextButton::textColourOnId, juce::Colours::black);
        setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (.85f));
        setColour (juce::ComboBox::backgroundColourId, panel.brighter (.1f));
        setColour (juce::ComboBox::outlineColourId, accent.withAlpha (.35f));
        setColour (juce::ToggleButton::tickColourId, accent);
        setColour (juce::ListBox::backgroundColourId, panel);
        setColour (juce::PopupMenu::backgroundColourId, panel);
    }
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider& s) override
    {
        auto r = juce::Rectangle<float> ((float) x, (float) y + (float) h * .5f - 3.f, (float) w, 6.f);
        g.setColour (juce::Colour (0xff23232e)); g.fillRoundedRectangle (r, 3.f);
        g.setColour (accent.withAlpha (s.isEnabled() ? 1.f : .4f)); g.fillRoundedRectangle (r.withRight (pos), 3.f);
        g.setColour (juce::Colours::white); g.fillEllipse (pos - 5.f, (float) y + (float) h * .5f - 5.f, 10.f, 10.f);
    }
};

// Builds sliders/toggles/combos from the shared parameter table for one workspace.
class ParamPanel : public juce::Component
{
public:
    ParamPanel (juce::AudioProcessorValueTreeState&, unsigned groupMask, juce::StringArray choiceIds = {});
    int getRequiredHeight (int width) const;
    void resized() override;
private:
    struct Row { std::unique_ptr<juce::Label> label; std::unique_ptr<juce::Slider> slider; std::unique_ptr<juce::ToggleButton> toggle;
                 std::unique_ptr<juce::ComboBox> combo;
                 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sa;
                 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba;
                 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ca; };
    std::vector<Row> rows;
    int columnsFor (int w) const { return juce::jmax (1, w / 300); }
};

class ImagePanel : public juce::Component, private juce::ChangeListener
{
public:
    ImagePanel (MonieProcessor&, std::function<void (juce::String)> status);
    ~ImagePanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); }
    juce::Rectangle<int> cellFor (int i) const;
    void browse (int replaceIndex);
    MonieProcessor& proc;
    std::function<void (juce::String)> status;
    juce::TextButton importBtn { "Import" }, removeBtn { "Remove" }, replaceBtn { "Replace" }, leftBtn { "<" }, rightBtn { ">" };
    std::unique_ptr<juce::FileChooser> chooser;
};

class AudioMeters : public juce::Component, private juce::Timer
{
public:
    explicit AudioMeters (MonieProcessor& p) : proc (p) { startTimerHz (30); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { repaint(); }
    MonieProcessor& proc;
    float beat = 0; uint32_t lastBeat = 0;
};

class PresetPanel : public juce::Component, private juce::ListBoxModel
{
public:
    explicit PresetPanel (MonieProcessor&);
    void resized() override;
    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override { loadRow (row); }
private:
    void refresh();
    void loadRow (int row);
    juce::File userFileForRow (int row) const;
    MonieProcessor& proc;
    juce::ListBox list;
    juce::TextButton loadBtn { "Load" }, saveBtn { "Save As..." }, dupBtn { "Duplicate" }, delBtn { "Delete" }, resetBtn { "Reset All" };
    juce::Array<juce::File> userFiles;
};

class MonieEditor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget,
                    private juce::Timer, private juce::ChangeListener, private juce::ListBoxModel
{
public:
    explicit MonieEditor (MonieProcessor&);
    ~MonieEditor() override;
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragging = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragging = false; repaint(); }
    void filesDropped (const juce::StringArray&, int, int) override;

private:
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    int getNumRows() override { return sceneNames().size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void selectedRowsChanged (int row) override;
    void showWorkspace (int index);
    void toggleFullscreen();
    void setStatus (const juce::String& s) { statusText = s; statusStamp = juce::Time::getMillisecondCounter(); }

    MonieProcessor& proc;
    MonieLookAndFeel laf;
    VisualizerComponent viz;

    juce::StringArray workspaceNames { "VISUALS", "IMAGES", "EFFECTS", "COLORS", "CUSTOMIZE", "AUDIO", "PRESETS", "EXPORT" };
    juce::OwnedArray<juce::TextButton> tabs;
    juce::Slider masterSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterAtt;
    juce::TextButton fullscreenBtn { "Fullscreen" }, randomBtn { "Random Scene" }, varyBtn { "Vary" }, resetAllBtn { "Reset All" };
    unsigned varySeed = 1;
    juce::ComboBox themeBox;
    juce::ListBox sceneList { "scenes", this };

    juce::OwnedArray<juce::Component> workspaces;
    juce::OwnedArray<ParamPanel> paramPanels;
    juce::Viewport viewport;
    std::unique_ptr<ImagePanel> imagePanel;
    std::unique_ptr<AudioMeters> meters;
    std::unique_ptr<PresetPanel> presetPanel;
    juce::Label exportInfo;
    int currentWorkspace = 0;
    bool previewOnly = false, dragging = false;
    juce::String statusText; juce::uint32 statusStamp = 0;
    juce::Image lastMain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonieEditor)
};
}
using MonieEditor = monie::MonieEditor;
