#include "PluginEditor.h"

using namespace monie;

// ============ ParamPanel ============
ParamPanel::ParamPanel (juce::AudioProcessorValueTreeState& apvts, unsigned mask, juce::StringArray choiceIds)
{
    for (auto& id : choiceIds)
    {
        auto* cp = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id));
        if (cp == nullptr) continue;
        Row r; r.label.reset (new juce::Label ({}, cp->getName (32))); r.combo.reset (new juce::ComboBox());
        r.combo->addItemList (cp->choices, 1);
        r.ca.reset (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (apvts, id, *r.combo));
        addAndMakeVisible (*r.label); addAndMakeVisible (*r.combo);
        rows.push_back (std::move (r));
    }
    for (auto& d : kParams)
    {
        if ((d.groups & mask) == 0) continue;
        Row r; r.label.reset (new juce::Label ({}, d.name));
        addAndMakeVisible (*r.label);
        if (d.toggle)
        {
            r.toggle.reset (new juce::ToggleButton ("On"));
            r.ba.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (apvts, d.id, *r.toggle));
            addAndMakeVisible (*r.toggle);
        }
        else
        {
            r.slider.reset (new juce::Slider (juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight));
            r.slider->setTextBoxStyle (juce::Slider::TextBoxRight, false, 52, 20);
            r.slider->setNumDecimalPlacesToDisplay (d.step >= 1.f ? 0 : 2);
            r.sa.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, d.id, *r.slider));
            r.slider->setDoubleClickReturnValue (true, d.defV);      // double-click = reset this control
            addAndMakeVisible (*r.slider);
        }
        rows.push_back (std::move (r));
    }
}
int ParamPanel::getRequiredHeight (int w) const
{
    const int cols = columnsFor (w), n = (int) rows.size();
    return ((n + cols - 1) / cols) * 34 + 12;
}
void ParamPanel::resized()
{
    const int cols = columnsFor (getWidth()), colW = getWidth() / cols;
    for (size_t i = 0; i < rows.size(); ++i)
    {
        auto cell = juce::Rectangle<int> ((int) (i % (size_t) cols) * colW, (int) (i / (size_t) cols) * 34 + 6, colW, 34).reduced (8, 3);
        rows[i].label->setBounds (cell.removeFromLeft (juce::jmin (130, cell.getWidth() / 2)));
        if (rows[i].slider) rows[i].slider->setBounds (cell);
        if (rows[i].toggle) rows[i].toggle->setBounds (cell);
        if (rows[i].combo)  rows[i].combo->setBounds (cell);
    }
}

// ============ ImagePanel ============
ImagePanel::ImagePanel (MonieProcessor& p, std::function<void (juce::String)> s) : proc (p), status (std::move (s))
{
    for (auto* b : { &importBtn, &removeBtn, &replaceBtn, &leftBtn, &rightBtn }) addAndMakeVisible (*b);
    importBtn.onClick = [this] { browse (-1); };
    replaceBtn.onClick = [this] { if (proc.library.getMain() >= 0) browse (proc.library.getMain()); };
    removeBtn.onClick = [this] { proc.library.remove (proc.library.getMain()); };
    leftBtn.onClick = [this] { auto m = proc.library.getMain(); if (m > 0) proc.library.move (m, m - 1); };
    rightBtn.onClick = [this] { auto m = proc.library.getMain(); if (m >= 0) proc.library.move (m, m + 1); };
    proc.library.addChangeListener (this);
}
ImagePanel::~ImagePanel() { proc.library.removeChangeListener (this); }
void ImagePanel::browse (int replaceIndex)
{
    chooser = std::make_unique<juce::FileChooser> (replaceIndex >= 0 ? "Replace / relink image" : "Import images", juce::File(), "*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.tga");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems,
        [this, replaceIndex] (const juce::FileChooser& fc)
        {
            auto results = fc.getResults();
            for (int i = 0; i < results.size(); ++i)
            {
                if (replaceIndex >= 0 && i == 0) proc.library.replace (replaceIndex, results[i]);
                else proc.library.addFile (results[i]);
            }
        });
}
juce::Rectangle<int> ImagePanel::cellFor (int i) const
{
    const int cw = 84, per = juce::jmax (1, getWidth() / cw);
    return { (i % per) * cw + 4, (i / per) * cw + 4, cw - 8, cw - 8 };
}
void ImagePanel::resized()
{
    auto r = getLocalBounds().removeFromBottom (30).reduced (4, 2);
    const int w = r.getWidth() / 5;
    for (auto* b : { &importBtn, &removeBtn, &replaceBtn, &leftBtn, &rightBtn }) b->setBounds (r.removeFromLeft (w).reduced (2, 0));
}
void ImagePanel::paint (juce::Graphics& g)
{
    auto& lib = proc.library;
    g.setColour (juce::Colour (0xff101018)); g.fillRoundedRectangle (getLocalBounds().toFloat().withTrimmedBottom (32), 6.f);
    if (lib.size() == 0)
    {
        g.setColour (juce::Colours::white.withAlpha (.5f)); g.setFont (14.f);
        g.drawFittedText ("Drag images from Explorer onto the preview,\nor press Import. Click a thumbnail to make it the main visual.", getLocalBounds().withTrimmedBottom (32), juce::Justification::centred, 3);
        return;
    }
    auto accent = dynamic_cast<MonieLookAndFeel*> (&getLookAndFeel()) ? static_cast<MonieLookAndFeel&> (getLookAndFeel()).accent : juce::Colours::cyan;
    for (int i = 0; i < lib.size(); ++i)
    {
        auto c = cellFor (i); auto& e = lib.get (i);
        g.setColour (juce::Colour (0xff1c1c26)); g.fillRoundedRectangle (c.toFloat(), 4.f);
        if (e.thumb.isValid()) g.drawImageWithin (e.thumb, c.getX() + 2, c.getY() + 2, c.getWidth() - 4, c.getHeight() - 4, juce::RectanglePlacement::centred);
        g.setFont (11.f);
        if (e.loading) { g.setColour (juce::Colours::white.withAlpha (.6f)); g.drawText ("loading...", c, juce::Justification::centred); }
        else if (e.missing) { g.setColour (juce::Colours::red); g.drawFittedText ("MISSING\n" + e.file.getFileName() + "\n(Replace to relink)", c, juce::Justification::centred, 4); }
        if (i == lib.getMain()) { g.setColour (accent); g.drawRoundedRectangle (c.toFloat(), 4.f, 2.f); }
    }
}
void ImagePanel::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < proc.library.size(); ++i)
        if (cellFor (i).contains (e.getPosition())) { proc.library.setMain (i); return; }
}

// ============ AudioMeters ============
void AudioMeters::paint (juce::Graphics& g)
{
    auto& A = proc.analysis;
    if (A.beatCount.load() != lastBeat) { lastBeat = A.beatCount.load(); beat = 1.f; }
    beat *= .85f;
    auto accent = static_cast<MonieLookAndFeel&> (getLookAndFeel()).accent;
    g.setColour (juce::Colour (0xff101018)); g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.f);
    auto area = getLocalBounds().reduced (10, 8);
    auto spec = area.removeFromTop (area.getHeight() * 2 / 3);
    const float bw = (float) spec.getWidth() / AnalysisFrame::numBands;
    for (int i = 0; i < AnalysisFrame::numBands; ++i)
    {
        const float v = A.bands[i].load();
        g.setColour (accent.withAlpha (.35f + .65f * v));
        g.fillRect (spec.getX() + i * bw, spec.getBottom() - v * spec.getHeight(), bw - 1.f, v * spec.getHeight());
    }
    const char* names[] = { "BASS", "MID", "TREB", "RMS", "PEAK", "BEAT" };
    const float vals[] = { A.bass.load(), A.mid.load(), A.treble.load(), A.rms.load(), A.peak.load(), beat };
    const float cw = (float) area.getWidth() / 6.f;
    g.setFont (11.f);
    for (int i = 0; i < 6; ++i)
    {
        auto c = juce::Rectangle<float> (area.getX() + i * cw + 4, (float) area.getY() + 14.f, cw - 8, 8.f);
        g.setColour (juce::Colour (0xff23232e)); g.fillRoundedRectangle (c, 3.f);
        g.setColour (i == 5 ? juce::Colours::white : accent); g.fillRoundedRectangle (c.withWidth (c.getWidth() * juce::jlimit (0.f, 1.f, vals[i])), 3.f);
        g.setColour (juce::Colours::white.withAlpha (.7f)); g.drawText (names[i], c.translated (0, -14).toNearestInt(), juce::Justification::centredLeft);
    }
}

// ============ PresetPanel ============
PresetPanel::PresetPanel (MonieProcessor& p) : proc (p)
{
    list.setModel (this); list.setRowHeight (24);
    addAndMakeVisible (list);
    for (auto* b : { &loadBtn, &saveBtn, &dupBtn, &delBtn, &resetBtn }) addAndMakeVisible (*b);
    loadBtn.onClick = [this] { loadRow (list.getSelectedRow()); };
    resetBtn.onClick = [this] { proc.resetAll(); };
    saveBtn.onClick = [this]
    {
        auto* aw = new juce::AlertWindow ("Save preset", "Preset name:", juce::MessageBoxIconType::NoIcon);
        aw->addTextEditor ("name", "My Preset");
        aw->addButton ("Save", 1); aw->addButton ("Cancel", 0);
        aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, aw] (int r)
            { if (r == 1) { proc.saveUserPreset (aw->getTextEditorContents ("name")); refresh(); } }), true);
    };
    dupBtn.onClick = [this] { auto f = userFileForRow (list.getSelectedRow()); if (f != juce::File()) { proc.duplicateUserPreset (f); refresh(); } };
    delBtn.onClick = [this] { auto f = userFileForRow (list.getSelectedRow()); if (f != juce::File()) { proc.deleteUserPreset (f); refresh(); } };
    refresh();
}
void PresetPanel::refresh() { userFiles = proc.getUserPresets(); list.updateContent(); list.repaint(); }
int PresetPanel::getNumRows() { return (int) factoryPresets().size() + userFiles.size(); }
juce::File PresetPanel::userFileForRow (int row) const
{
    const int i = row - (int) factoryPresets().size();
    return juce::isPositiveAndBelow (i, userFiles.size()) ? userFiles[i] : juce::File();
}
void PresetPanel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool sel)
{
    auto accent = static_cast<MonieLookAndFeel&> (getLookAndFeel()).accent;
    if (sel) { g.setColour (accent.withAlpha (.25f)); g.fillRect (0, 0, w, h); }
    const bool factory = row < (int) factoryPresets().size();
    g.setColour (juce::Colours::white.withAlpha (.9f)); g.setFont (14.f);
    g.drawText ((factory ? "Factory  |  " : "User  |  ") + (factory ? juce::String (factoryPresets()[(size_t) row].name) : userFileForRow (row).getFileNameWithoutExtension()), 10, 0, w - 10, h, juce::Justification::centredLeft);
}
void PresetPanel::loadRow (int row)
{
    if (row < 0) return;
    if (row < (int) factoryPresets().size()) proc.applyFactoryPreset (row);
    else proc.loadUserPreset (userFileForRow (row));
}
void PresetPanel::resized()
{
    auto r = getLocalBounds().reduced (6);
    auto bar = r.removeFromBottom (32);
    const int w = bar.getWidth() / 5;
    for (auto* b : { &loadBtn, &saveBtn, &dupBtn, &delBtn, &resetBtn }) b->setBounds (bar.removeFromLeft (w).reduced (2));
    list.setBounds (r);
}

// ============ MonieEditor ============
MonieEditor::MonieEditor (MonieProcessor& p) : juce::AudioProcessorEditor (p), proc (p), viz (p)
{
    setLookAndFeel (&laf);
    laf.setTheme (proc.getTheme());
    addAndMakeVisible (viz);

    for (int i = 0; i < workspaceNames.size(); ++i)
    {
        auto* b = tabs.add (new juce::TextButton (workspaceNames[i]));
        b->setClickingTogglesState (true); b->setRadioGroupId (1);
        b->onClick = [this, i] { showWorkspace (i); };
        addAndMakeVisible (b);
    }
    masterAtt.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (proc.apvts, "master", masterSlider));
    addAndMakeVisible (masterSlider);
    for (auto* b : { &fullscreenBtn, &randomBtn, &varyBtn, &resetAllBtn }) addAndMakeVisible (*b);
    varyBtn.setTooltip ("Reproducible variation of all sliders (seed increments each click)");
    varyBtn.onClick = [this] { proc.varyParams (varySeed++, 0.15f); };
    fullscreenBtn.onClick = [this] { toggleFullscreen(); };
    randomBtn.onClick = [this] { proc.setParam ("scene", (float) juce::Random::getSystemRandom().nextInt (sceneNames().size())); };
    resetAllBtn.onClick = [this] { proc.resetAll(); };
    themeBox.addItemList ({ "Neon", "Grayscale", "High Contrast" }, 1);
    themeBox.setSelectedItemIndex (proc.getTheme(), juce::dontSendNotification);
    themeBox.onChange = [this]
    {
        proc.setTheme (themeBox.getSelectedItemIndex()); laf.setTheme (themeBox.getSelectedItemIndex());
        sendLookAndFeelChange(); repaint();
    };
    addAndMakeVisible (themeBox);

    sceneList.setRowHeight (44); sceneList.setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (sceneList);

    // workspaces (each one is a ParamPanel, optionally with a header component, inside the shared viewport)
    auto add = [this] (unsigned mask, juce::StringArray choices) { auto* pp = paramPanels.add (new ParamPanel (proc.apvts, mask, choices)); return pp; };
    add (GVis, { "scene" });                  // 0 VISUALS
    add (GImg, {});                           // 1 IMAGES (image library header added in resized)
    add (GFx, {});                            // 2 EFFECTS
    add (GCol, { "palette" });                // 3 COLORS
    add (GCus, { "quality", "fps" });         // 4 CUSTOMIZE
    add (GAud, {});                           // 5 AUDIO
    imagePanel.reset (new ImagePanel (proc, [this] (juce::String s) { setStatus (s); }));
    meters.reset (new AudioMeters (proc));
    presetPanel.reset (new PresetPanel (proc));
    exportInfo.setJustificationType (juce::Justification::topLeft);
    exportInfo.setFont (juce::Font (juce::FontOptions (14.f)));
    exportInfo.setText ("VIDEO EXPORT - status: NOT IMPLEMENTED in this build\n\n"
                        "To record today: open the Standalone app (or the plugin window), press Fullscreen, and capture it with OBS Studio "
                        "(Window Capture / Display Capture, NVENC H.264, MP4 or MKV->MP4). Add your DAW audio via OBS Desktop Audio.\n\n"
                        "Planned: standalone renderer that pipes frames to FFmpeg (ffmpeg.exe on PATH) with a WAV/audio file for exact sync, "
                        "1080p/4K, quality and frame-rate selection. See README 'Roadmap'.", juce::dontSendNotification);
    addChildComponent (viewport); addChildComponent (*imagePanel); addChildComponent (*meters);
    addChildComponent (*presetPanel); addChildComponent (exportInfo);
    viewport.setScrollBarsShown (true, false);

    proc.library.addChangeListener (this);
    changeListenerCallback (&proc.library);

    setWantsKeyboardFocus (true);
    setResizable (true, true);
    setResizeLimits (960, 600, 3840, 2160);
    setSize (1280, 760);
    tabs[0]->setToggleState (true, juce::dontSendNotification);
    showWorkspace (0);
    startTimerHz (20);
}

MonieEditor::~MonieEditor()
{
    proc.library.removeChangeListener (this);
    if (juce::JUCEApplicationBase::isStandaloneApp()) juce::Desktop::getInstance().setKioskModeComponent (nullptr);
    setLookAndFeel (nullptr);
}

void MonieEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    auto img = proc.library.getMainImage();
    if (! (img == lastMain)) { lastMain = img; viz.setMainImage (img); }
}

void MonieEditor::showWorkspace (int index)
{
    currentWorkspace = index;
    const bool isParam = index <= 5, hasImages = index == 1, hasMeters = index == 5;
    viewport.setVisible (isParam);
    imagePanel->setVisible (hasImages); meters->setVisible (hasMeters);
    presetPanel->setVisible (index == 6); exportInfo.setVisible (index == 7);
    if (isParam) viewport.setViewedComponent (paramPanels[index], false);
    resized();
}

void MonieEditor::toggleFullscreen()
{
    previewOnly = ! previewOnly;
    if (juce::JUCEApplicationBase::isStandaloneApp())
        juce::Desktop::getInstance().setKioskModeComponent (previewOnly ? getTopLevelComponent() : nullptr, false);
    resized(); grabKeyboardFocus();
}
bool MonieEditor::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey && previewOnly) { toggleFullscreen(); return true; }
    if (k == juce::KeyPress::F11Key) { toggleFullscreen(); return true; }
    return false;
}

void MonieEditor::resized()
{
    auto r = getLocalBounds();
    for (auto* c : std::initializer_list<juce::Component*> { &masterSlider, &fullscreenBtn, &randomBtn, &resetAllBtn, &themeBox, &sceneList, &viewport, imagePanel.get(), meters.get(), presetPanel.get(), &exportInfo })
        if (c != nullptr && previewOnly) c->setVisible (false);
    for (auto* t : tabs) t->setVisible (! previewOnly);
    if (previewOnly) { viz.setBounds (r); return; }

    masterSlider.setVisible (true); fullscreenBtn.setVisible (true); randomBtn.setVisible (true); resetAllBtn.setVisible (true);
    themeBox.setVisible (true); sceneList.setVisible (true);
    const bool isParam = currentWorkspace <= 5;
    viewport.setVisible (isParam); imagePanel->setVisible (currentWorkspace == 1); meters->setVisible (currentWorkspace == 5);
    presetPanel->setVisible (currentWorkspace == 6); exportInfo.setVisible (currentWorkspace == 7);

    auto top = r.removeFromTop (52).reduced (10, 8);
    top.removeFromLeft (170);                                    // logo area (painted)
    auto right = top.removeFromRight (520);
    themeBox.setBounds (right.removeFromRight (110).reduced (2, 0));
    resetAllBtn.setBounds (right.removeFromRight (80).reduced (2, 0));
    fullscreenBtn.setBounds (right.removeFromRight (90).reduced (2, 0));
    masterSlider.setBounds (right.reduced (6, 0));
    const int tw = juce::jmin (96, top.getWidth() / tabs.size());
    for (auto* t : tabs) t->setBounds (top.removeFromLeft (tw).reduced (2, 0));

    r.removeFromBottom (22);                                    // status bar
    auto left = r.removeFromLeft (200).reduced (6, 4);
    auto rb = left.removeFromBottom (30).reduced (0, 2);
    varyBtn.setBounds (rb.removeFromRight (rb.getWidth() / 3).reduced (2, 0));
    randomBtn.setBounds (rb);
    sceneList.setBounds (left);

    const int panelW = juce::jlimit (340, 520, getWidth() / 3);
    auto panel = r.removeFromRight (panelW).reduced (6, 4);
    viz.setBounds (r.reduced (4));

    auto content = panel;
    if (currentWorkspace == 1) imagePanel->setBounds (content.removeFromTop (190));
    if (currentWorkspace == 5) meters->setBounds (content.removeFromTop (130).reduced (0, 4));
    viewport.setBounds (content);
    presetPanel->setBounds (panel); exportInfo.setBounds (panel.reduced (8));
    if (isParam)
    {
        auto* pp = paramPanels[currentWorkspace];
        const int w = viewport.getWidth() - viewport.getScrollBarThickness();
        pp->setSize (w, juce::jmax (viewport.getHeight(), pp->getRequiredHeight (w)));
    }
}

void MonieEditor::paint (juce::Graphics& g)
{
    g.fillAll (laf.bg);
    if (previewOnly) return;
    g.setColour (laf.panel); g.fillRect (getLocalBounds().withTrimmedTop (52));
    g.setColour (laf.accent); g.setFont (juce::FontOptions (22.f, juce::Font::bold));
    g.drawText ("monie visuals", 14, 8, 170, 36, juce::Justification::centredLeft);
    g.setColour (juce::Colours::white.withAlpha (.55f)); g.setFont (12.f);
    auto bar = getLocalBounds().removeFromBottom (22);
    juce::String s = juce::String (viz.getFps(), 0) + " FPS render  |  images: " + juce::String (proc.library.size());
    if (statusText.isNotEmpty() && juce::Time::getMillisecondCounter() - statusStamp < 6000) s = statusText;
    g.drawText (s, bar.reduced (10, 0), juce::Justification::centredLeft);
    g.drawText ("Esc/F11: exit fullscreen  |  double-click a slider to reset it", bar.reduced (10, 0), juce::Justification::centredRight);
}

void MonieEditor::paintOverChildren (juce::Graphics& g)
{
    auto err = viz.getError();
    if (err.isNotEmpty())
    {
        g.setColour (juce::Colours::black.withAlpha (.8f)); g.fillRect (viz.getBounds());
        g.setColour (juce::Colours::orangered); g.setFont (13.f);
        g.drawFittedText ("GPU / shader problem - rendering is disabled (audio passthrough is unaffected).\n\n" + err, viz.getBounds().reduced (16), juce::Justification::topLeft, 30);
    }
    if (dragging)
    {
        auto b = viz.getBounds().toFloat().reduced (6);
        g.setColour (juce::Colours::black.withAlpha (.55f)); g.fillRect (viz.getBounds());
        g.setColour (laf.accent); g.drawRoundedRectangle (b, 10.f, 3.f);
        g.setFont (juce::FontOptions (22.f, juce::Font::bold));
        g.drawText ("Drop images to import", b.toNearestInt(), juce::Justification::centred);
    }
}

bool MonieEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files) if (ImageLibrary::looksLikeImage (juce::File (f))) return true;
    return false;
}
void MonieEditor::filesDropped (const juce::StringArray& files, int, int)
{
    dragging = false;
    juce::StringArray msgs; int added = 0;
    for (auto& f : files)
    {
        juce::File file (f);
        auto why = ImageLibrary::unsupportedReason (file);
        if (why.isEmpty()) { proc.library.addFile (file); ++added; }
        else msgs.add (file.getFileName() + ": " + why);
    }
    setStatus (msgs.isEmpty() ? "Imported " + juce::String (added) + " image(s)" : msgs.joinIntoString ("  "));
    repaint();
}

void MonieEditor::timerCallback()
{
    const int sc = (int) *proc.apvts.getRawParameterValue ("scene");
    if (sceneList.getSelectedRow() != sc) sceneList.selectRow (sc, true, false);
    repaint (0, getHeight() - 22, getWidth(), 22);
    if (viz.getError().isNotEmpty()) repaint();
}

void MonieEditor::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool sel)
{
    // procedural thumbnail: palette gradient with a scene-specific pattern
    const int pi = juce::jlimit (0, 11, (int) *proc.apvts.getRawParameterValue ("palette"));
    const auto& pd = kPalettes[pi];
    auto col = [&] (int k) { return juce::Colour::fromFloatRGBA (pd.c[k * 3], pd.c[k * 3 + 1], pd.c[k * 3 + 2], 1.f); };
    auto thumb = juce::Rectangle<float> (4.f, 4.f, (float) h - 8.f, (float) h - 8.f) .withWidth (62.f);
    g.setGradientFill (juce::ColourGradient (col (0), thumb.getTopLeft(), col (2), thumb.getBottomRight(), false));
    g.fillRoundedRectangle (thumb, 4.f);
    g.setColour (col (2).withAlpha (.8f));
    juce::Random rnd (row * 977 + 13);
    for (int i = 0; i < 6; ++i)
    {
        auto x = thumb.getX() + rnd.nextFloat() * thumb.getWidth(), y = thumb.getY() + rnd.nextFloat() * thumb.getHeight();
        if (row % 3 == 0) g.drawEllipse (x - 8, y - 8, 16, 16, 1.f); else if (row % 3 == 1) g.drawLine (x, thumb.getY(), x + 10, thumb.getBottom(), 1.f); else g.fillRect (x, y, 6.f, 6.f);
    }
    if (sel) { g.setColour (laf.accent); g.drawRoundedRectangle (thumb, 4.f, 2.f); }
    g.setColour (juce::Colours::white.withAlpha (sel ? 1.f : .75f)); g.setFont (13.f);
    g.drawText (juce::String (row + 1) + ". " + sceneNames()[row], 72, 0, w - 74, h, juce::Justification::centredLeft);
}
void MonieEditor::selectedRowsChanged (int row)
{
    // only write when it actually differs: avoids fighting host automation of the Scene parameter
    if (row >= 0 && row != (int) *proc.apvts.getRawParameterValue ("scene")) proc.setParam ("scene", (float) row);
}
