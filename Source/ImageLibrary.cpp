#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "ImageLibrary.h"
#include <stb_image.h>

namespace monie
{
static const char* const kSupported[] = { "png", "jpg", "jpeg", "bmp", "gif", "tga" };
static const char* const kUnsupportedKnown[] = { "webp", "tif", "tiff" };

ImageLibrary::ImageLibrary() {}
ImageLibrary::~ImageLibrary() { pool.removeAllJobs (true, 3000); }

bool ImageLibrary::looksLikeImage (const juce::File& f)
{
    const auto e = f.getFileExtension().trimCharactersAtStart (".").toLowerCase();
    for (auto* s : kSupported) if (e == s) return true;
    for (auto* s : kUnsupportedKnown) if (e == s) return true;
    return false;
}

juce::String ImageLibrary::unsupportedReason (const juce::File& f)
{
    const auto e = f.getFileExtension().trimCharactersAtStart (".").toLowerCase();
    for (auto* s : kSupported) if (e == s) return {};
    if (e == "webp") return "WebP is not supported in this build (convert to PNG/JPG).";
    if (e == "tif" || e == "tiff") return "TIFF is not supported in this build (convert to PNG/JPG).";
    return "Unsupported file type: ." + e;
}

void ImageLibrary::addFile (const juce::File& f)
{
    if (unsupportedReason (f).isNotEmpty()) return;
    Entry e; e.uid = nextUid++; e.file = f; e.missing = ! f.existsAsFile();
    e.loading = ! e.missing;
    entries.add (e);
    if (mainIndex < 0) mainIndex = entries.size() - 1;
    if (! e.missing) startDecode (e.uid, f);
    sendChangeMessage();
}

void ImageLibrary::remove (int i)
{
    if (! juce::isPositiveAndBelow (i, entries.size())) return;
    entries.remove (i);
    if (entries.isEmpty()) mainIndex = -1;
    else if (mainIndex == i) mainIndex = juce::jmin (i, entries.size() - 1);
    else if (mainIndex > i) --mainIndex;
    sendChangeMessage();
}

void ImageLibrary::replace (int i, const juce::File& f)
{
    if (! juce::isPositiveAndBelow (i, entries.size()) || unsupportedReason (f).isNotEmpty()) return;
    auto& e = entries.getReference (i);
    e.file = f; e.full = {}; e.thumb = {}; e.missing = ! f.existsAsFile(); e.loading = ! e.missing;
    if (! e.missing) startDecode (e.uid, f);
    sendChangeMessage();
}

void ImageLibrary::move (int from, int to)
{
    if (! juce::isPositiveAndBelow (from, entries.size()) || ! juce::isPositiveAndBelow (to, entries.size()) || from == to) return;
    const int mainUid = mainIndex >= 0 ? entries[mainIndex].uid : -1;
    entries.move (from, to);
    if (mainUid >= 0) mainIndex = indexOfUid (mainUid);
    sendChangeMessage();
}

void ImageLibrary::setMain (int i)
{
    if (juce::isPositiveAndBelow (i, entries.size()) && i != mainIndex) { mainIndex = i; sendChangeMessage(); }
}

juce::Image ImageLibrary::getMainImage() const
{
    return juce::isPositiveAndBelow (mainIndex, entries.size()) ? entries.getReference (mainIndex).full : juce::Image();
}

juce::StringArray ImageLibrary::getPaths() const
{
    juce::StringArray s; for (auto& e : entries) s.add (e.file.getFullPathName()); return s;
}

void ImageLibrary::setFiles (const juce::StringArray& paths, int main)
{
    entries.clear(); mainIndex = -1;
    for (auto& p : paths) addFile (juce::File (p));
    if (juce::isPositiveAndBelow (main, entries.size())) mainIndex = main;
    sendChangeMessage();
}

int ImageLibrary::indexOfUid (int uid) const
{
    for (int i = 0; i < entries.size(); ++i) if (entries.getReference (i).uid == uid) return i;
    return -1;
}

static juce::Image decodeFile (const juce::File& f)
{
    juce::MemoryBlock mb;
    if (! f.loadFileAsData (mb) || mb.getSize() == 0 || mb.getSize() > (size_t) 256 * 1024 * 1024) return {};
    int w = 0, h = 0, n = 0;
    if (! stbi_info_from_memory ((const stbi_uc*) mb.getData(), (int) mb.getSize(), &w, &h, &n)) return {};
    if (w <= 0 || h <= 0 || (long long) w * h > 100000000LL) return {};   // refuse > 100 MP
    auto* px = stbi_load_from_memory ((const stbi_uc*) mb.getData(), (int) mb.getSize(), &w, &h, &n, 4);
    if (px == nullptr) return {};
    juce::Image img (juce::Image::ARGB, w, h, false);
    {
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < h; ++y)
        {
            auto* line = (juce::PixelARGB*) bd.getLinePointer (y);
            const unsigned char* src = px + (size_t) y * (size_t) w * 4;
            for (int x = 0; x < w; ++x, src += 4) { line[x].setARGB (src[3], src[0], src[1], src[2]); line[x].premultiply(); }
        }
    }
    stbi_image_free (px);
    const int maxDim = 2048;   // GPU memory / upload-time cap
    if (w > maxDim || h > maxDim)
    {
        const float s = (float) maxDim / (float) juce::jmax (w, h);
        img = img.rescaled (juce::jmax (1, (int) (w * s)), juce::jmax (1, (int) (h * s)), juce::Graphics::highResamplingQuality);
    }
    return img;
}

void ImageLibrary::startDecode (int uid, const juce::File& file)
{
    juce::WeakReference<ImageLibrary> weak (this);
    pool.addJob ([weak, uid, file]
    {
        auto full = decodeFile (file);
        juce::Image thumb;
        if (full.isValid())
        {
            const float s = 96.f / (float) juce::jmax (full.getWidth(), full.getHeight());
            thumb = full.rescaled (juce::jmax (1, (int) (full.getWidth() * s)), juce::jmax (1, (int) (full.getHeight() * s)), juce::Graphics::mediumResamplingQuality);
        }
        juce::MessageManager::callAsync ([weak, uid, full, thumb]
        {
            if (auto* self = weak.get())
            {
                const int i = self->indexOfUid (uid);
                if (i < 0) return;
                auto& e = self->entries.getReference (i);
                e.loading = false; e.full = full; e.thumb = thumb; e.missing = ! full.isValid();
                self->sendChangeMessage();
            }
        });
    });
}
} // namespace monie
