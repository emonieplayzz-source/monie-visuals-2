#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_events/juce_events.h>

namespace monie
{
// Message-thread image collection. Decoding happens on a worker thread (never blocks the UI or the audio thread).
class ImageLibrary : public juce::ChangeBroadcaster
{
public:
    struct Entry { int uid = 0; juce::File file; juce::Image full, thumb; bool loading = true, missing = false; };

    ImageLibrary();
    ~ImageLibrary() override;

    static bool looksLikeImage (const juce::File&);
    // empty string = supported. Otherwise a human readable reason.
    static juce::String unsupportedReason (const juce::File&);

    void addFile (const juce::File&);
    void remove (int index);
    void replace (int index, const juce::File&);   // also used to relink missing files
    void move (int from, int to);
    void setMain (int index);
    int  getMain() const { return mainIndex; }
    int  size() const { return entries.size(); }
    const Entry& get (int i) const { return entries.getReference (i); }
    juce::Image getMainImage() const;
    juce::StringArray getPaths() const;
    void setFiles (const juce::StringArray& paths, int main);

private:
    void startDecode (int uid, const juce::File&);
    int indexOfUid (int uid) const;

    juce::Array<Entry> entries;
    int mainIndex = -1, nextUid = 1;
    juce::ThreadPool pool { 1 };
    JUCE_DECLARE_WEAK_REFERENCEABLE (ImageLibrary)
};
} // namespace monie
