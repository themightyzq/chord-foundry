#include "ProjectFile.h"

#include <cmath>

namespace ChordFoundry {

const char* const ProjectFile::fileExtension = ".cfproj";
const char* const ProjectFile::fileWildcard = "*.cfproj";

namespace {

const juce::Identifier rootTag("ChordFoundryProject");
const juce::Identifier settingsTag("Settings");
const juce::Identifier progressionTag("ChordProgression");

float finiteOr(double value, float fallback)
{
    return std::isfinite(value) ? static_cast<float>(value) : fallback;
}

} // namespace

juce::String ProjectFile::toXmlString(const ProjectSettings& settings, const ChordProgression& progression)
{
    juce::ValueTree root(rootTag);
    root.setProperty("formatVersion", currentFormatVersion, nullptr);

    juce::ValueTree settingsTree(settingsTag);
    // A float widened to double and printed with enough digits reads back as the same float.
    settingsTree.setProperty("tempo", juce::String(static_cast<double>(settings.tempo), 9), nullptr);
    settingsTree.setProperty("key", settings.key, nullptr);
    settingsTree.setProperty("mode", settings.mode, nullptr);
    settingsTree.setProperty("loop", settings.loop, nullptr);
    settingsTree.setProperty("clickTrack", settings.clickTrack, nullptr);
    settingsTree.setProperty("masterVolume", juce::String(static_cast<double>(settings.masterVolume), 9), nullptr);
    root.appendChild(settingsTree, nullptr);

    root.appendChild(progression.toValueTree(), nullptr);

    if (auto xml = root.createXml())
        return xml->toString();

    return {};
}

juce::Result ProjectFile::save(const juce::File& file,
                               const ProjectSettings& settings,
                               const ChordProgression& progression)
{
    const auto text = toXmlString(settings, progression);
    if (text.isEmpty())
        return juce::Result::fail("The project could not be converted to text.");

    // Write to a temporary file in the same folder, then swap it over the target. The target
    // is only touched by the swap, so a failure at any earlier point leaves it as it was.
    juce::TemporaryFile temp(file);

    {
        auto stream = temp.getFile().createOutputStream();
        if (stream == nullptr || stream->getStatus().failed())
            return juce::Result::fail("Could not create a file in " + file.getParentDirectory().getFullPathName()
                                      + ". Check that the folder exists and that you can write to it.");

        const auto* data = text.toRawUTF8();
        const auto numBytes = text.getNumBytesAsUTF8();

        if (!stream->write(data, numBytes))
            return juce::Result::fail("Writing the project file failed (disk full?).");

        stream->flush();
        if (stream->getStatus().failed())
            return juce::Result::fail("Writing the project file failed: " + stream->getStatus().getErrorMessage());

        stream.reset(); // close before the swap

        if (temp.getFile().getSize() != static_cast<juce::int64>(numBytes))
            return juce::Result::fail("The project file was not written completely.");
    }

    if (!temp.overwriteTargetFileWithTemporary())
        return juce::Result::fail("Could not replace " + file.getFullPathName()
                                  + ". The previous version of the file was left unchanged.");

    return juce::Result::ok();
}

juce::Result ProjectFile::load(const juce::File& file,
                               ProjectSettings& settings,
                               ChordProgression& progression)
{
    if (!file.existsAsFile())
        return juce::Result::fail("The file " + file.getFileName() + " does not exist.");

    juce::FileInputStream in(file);
    if (!in.openedOk())
        return juce::Result::fail("Could not open " + file.getFileName() + " for reading.");

    return parse(in.readEntireStreamAsString(), settings, progression);
}

juce::Result ProjectFile::parse(const juce::String& xmlText,
                                ProjectSettings& settings,
                                ChordProgression& progression)
{
    const auto xml = juce::XmlDocument::parse(xmlText);
    if (xml == nullptr || !xml->hasTagName(rootTag.toString()))
        return juce::Result::fail("This is not a Chord Foundry project file.");

    const auto root = juce::ValueTree::fromXml(*xml);

    const int version = root.getProperty("formatVersion", 0);
    if (version < 1)
        return juce::Result::fail("This is not a Chord Foundry project file (no format version).");
    if (version > currentFormatVersion)
        return juce::Result::fail("This project was saved by a newer version of Chord Foundry (file format "
                                  + juce::String(version) + "; this version reads up to "
                                  + juce::String(currentFormatVersion) + "). Update Chord Foundry to open it.");

    const auto settingsTree = root.getChildWithName(settingsTag);
    const auto progressionTree = root.getChildWithName(progressionTag);
    if (!settingsTree.isValid() || !progressionTree.isValid())
        return juce::Result::fail("The project file is incomplete (missing settings or chords).");

    // Everything below works on local copies; the caller's objects change only at the end.
    ProjectSettings loaded;
    loaded.tempo = juce::jlimit(40.0f, 240.0f,
                                finiteOr(static_cast<double>(settingsTree.getProperty("tempo", 120.0).toString().getDoubleValue()), 120.0f));
    loaded.key = settingsTree.getProperty("key", "C").toString();
    loaded.mode = settingsTree.getProperty("mode", "Major").toString();
    loaded.loop = static_cast<bool>(settingsTree.getProperty("loop", false));
    loaded.clickTrack = static_cast<bool>(settingsTree.getProperty("clickTrack", false));
    loaded.masterVolume = juce::jlimit(0.0f, 1.0f,
                                       finiteOr(settingsTree.getProperty("masterVolume", 0.7).toString().getDoubleValue(), 0.7f));

    if (loaded.key.isEmpty())
        loaded.key = "C";
    if (loaded.mode.isEmpty())
        loaded.mode = "Major";

    ChordProgression loadedProgression;
    loadedProgression.fromValueTree(progressionTree);

    settings = loaded;
    progression.fromValueTree(loadedProgression.toValueTree());
    return juce::Result::ok();
}

} // namespace ChordFoundry
