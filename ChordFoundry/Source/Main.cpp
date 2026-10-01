#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <cstdio>
#include "MainComponent.h"

//==============================================================================
class ChordFoundryApplication : public juce::JUCEApplication
{
public:
    //==============================================================================
    ChordFoundryApplication() {}

    const juce::String getApplicationName() override { return "Chord Foundry"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    //==============================================================================
    void initialise(const juce::String& commandLine) override
    {
        const auto args = juce::StringArray::fromTokens(commandLine, true);

        // Developer tool: render the window to a PNG without opening a window or the audio
        // device.  ChordFoundry --snapshot out.png 1200 732 [project.cfproj]
        if (args.contains("--snapshot"))
        {
            runSnapshot(args);
            return;
        }

        // Developer tool: drive the project/save/dirty logic of the main component without a
        // window or audio device and print PASS/FAIL lines.  ChordFoundry --project-selfcheck x.cfproj
        if (args.contains("--project-selfcheck"))
        {
            runProjectSelfCheck(args);
            return;
        }

        mainWindow.reset(new MainWindow(getApplicationName()));
        mainWindow->getMainComponent()->startAudio();

        // A project path on the command line is opened at launch.
        for (const auto& arg : args)
        {
            const auto file = juce::File::getCurrentWorkingDirectory().getChildFile(arg.unquoted());
            if (file.existsAsFile() && file.hasFileExtension(ChordFoundry::ProjectFile::fileExtension))
                mainWindow->getMainComponent()->openProjectFile(file);
        }
    }

    void shutdown() override
    {
        // Add your application's shutdown code here..
        mainWindow = nullptr; // (deletes our window)
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        // Quit, the window's close button and Cmd+Q all come through here: offer to save first.
        if (mainWindow == nullptr)
        {
            quit();
            return;
        }

        mainWindow->getMainComponent()->confirmDiscardChanges([](bool proceed)
        {
            if (proceed)
                quit();
        });
    }

    void anotherInstanceStarted(const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
    }

    //==============================================================================
    /*
        This class implements the desktop window that contains an instance of
        our MainComponent class.
    */
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name)
            : DocumentWindow(name,
                           juce::Desktop::getInstance().getDefaultLookAndFeel()
                                                      .findColour(juce::ResizableWindow::backgroundColourId),
                           DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);

            mainComponent = new ChordFoundry::MainComponent();
            setContentOwned(mainComponent, true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
           #else
            setResizable(true, true);
            
            // Smallest window the layout is built for: fits a 13-inch laptop screen. Below
            // the content size the panels scroll rather than clip.
            setResizeLimits(1200, 760, 3840, 2400);
            
            // Default size, kept inside the usable area of the screen it opens on.
            const auto area = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay() != nullptr
                                  ? juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea
                                  : juce::Rectangle<int>(0, 0, 1280, 820);
            centreWithSize(juce::jmin(1280, area.getWidth()), juce::jmin(820, area.getHeight()));
           #endif

            setVisible(true);
        }

        ChordFoundry::MainComponent* getMainComponent() const { return mainComponent; }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        ChordFoundry::MainComponent* mainComponent = nullptr; // owned by the window

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    void runProjectSelfCheck(const juce::StringArray& args)
    {
        using namespace ChordFoundry;

        const int i = args.indexOf("--project-selfcheck");
        int failures = 0;

        const auto check = [&failures](bool ok, const juce::String& what)
        {
            std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what.toRawUTF8());
            if (! ok)
                ++failures;
        };

        if (args.size() < i + 2)
        {
            std::fprintf(stderr, "usage: --project-selfcheck <project.cfproj>\n");
            setApplicationReturnValue(2);
            quit();
            return;
        }

        const auto source = juce::File::getCurrentWorkingDirectory().getChildFile(args[i + 1].unquoted());
        const auto scratch = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                 .getNonexistentChildFile("ChordFoundrySelfCheck", "", false);
        scratch.createDirectory();
        const auto working = scratch.getChildFile("working.cfproj");
        source.copyFileTo(working);

        {
            MainComponent c;
            c.setSize(1200, 732);
            check(! c.hasUnsavedChanges(), "a new window has no unsaved changes");

            check(c.openProjectFile(working), "open the project");
            check(! c.hasUnsavedChanges(), "just opened: clean");
            check(c.getCurrentProjectFile() == working, "the opened file is the current file");

            ProjectSettings before;
            ChordProgression beforeProgression;
            check(ProjectFile::load(working, before, beforeProgression).wasOk(), "the file loads");

            c.onTempoChanged(150.0f);
            check(c.hasUnsavedChanges(), "changing the tempo marks the project changed");
            c.onTempoChanged(before.tempo);
            check(! c.hasUnsavedChanges(), "changing it back clears the mark");

            c.onKeyChanged("D");
            check(c.hasUnsavedChanges(), "key change marks the project changed");
            c.onModeChanged("Dorian");
            c.onLoopChanged(! before.loop);
            c.onClickTrackChanged(! before.clickTrack);
            c.onVolumeChanged(0.31f);
            c.onChordSelected("V");
            c.onChordAdded();
            c.onBlockAdded(BlockData(0, 24, 2, juce::Colours::grey));
            check(beforeProgression.getChordCount() + 1 == c.getChordCountForCheck(), "a chord was added");

            bool saved = false;
            c.saveProject([&saved](bool ok) { saved = ok; });
            check(saved, "Save writes to the open file");
            check(! c.hasUnsavedChanges(), "saved: clean");

            ProjectSettings after;
            ChordProgression afterProgression;
            check(ProjectFile::load(working, after, afterProgression).wasOk(), "the saved file loads");
            check(after.key == "D" && after.mode == "Dorian" && after.masterVolume == 0.31f
                      && after.loop == ! before.loop && after.clickTrack == ! before.clickTrack,
                  "settings were saved");
            check(afterProgression.getChordCount() == beforeProgression.getChordCount() + 1
                      && afterProgression.getBlockCount() == beforeProgression.getBlockCount() + 1,
                  "chords and blocks were saved");

            // A second window opening the saved file ends up in the same state.
            MainComponent d;
            d.setSize(1200, 732);
            check(d.openProjectFile(working) && ! d.hasUnsavedChanges(), "reopening the saved file works and is clean");
            check(d.getChordCountForCheck() == afterProgression.getChordCount(), "reopened chord count matches");

            // Garbage is refused and nothing changes.
            const auto bad = scratch.getChildFile("bad.cfproj");
            bad.replaceWithText("not a project");
            check(! d.openProjectFile(bad) && ! d.hasUnsavedChanges()
                      && d.getChordCountForCheck() == afterProgression.getChordCount(),
                  "a bad file is refused and the open project is untouched");

            // New project with nothing unsaved resets without asking.
            d.newProject();
            check(d.getChordCountForCheck() == 0 && d.getCurrentProjectFile() == juce::File() && ! d.hasUnsavedChanges(),
                  "New starts an empty, clean, untitled project");

            c.onTempoChanged(99.0f);
            check(c.hasUnsavedChanges(), "edit after save marks the project changed again");
        }

        scratch.deleteRecursively();
        std::printf("%d failure(s)\n", failures);
        setApplicationReturnValue(failures == 0 ? 0 : 1);
        quit();
    }

    void runSnapshot(const juce::StringArray& args)
    {
        const int i = args.indexOf("--snapshot");

        if (args.size() < i + 4)
        {
            std::fprintf(stderr, "usage: --snapshot <out.png> <width> <height> [project.cfproj]\n");
            setApplicationReturnValue(2);
            quit();
            return;
        }

        const auto out = juce::File::getCurrentWorkingDirectory().getChildFile(args[i + 1].unquoted());
        const int width = args[i + 2].getIntValue();
        const int height = args[i + 3].getIntValue();

        int result = 0;

        {
            ChordFoundry::MainComponent component;
            component.setSize(width, height);

            if (args.size() > i + 4)
            {
                const auto project = juce::File::getCurrentWorkingDirectory().getChildFile(args[i + 4].unquoted());
                if (! component.openProjectFile(project))
                {
                    std::fprintf(stderr, "could not open %s\n", project.getFullPathName().toRawUTF8());
                    result = 3;
                }
            }

            const auto image = component.createComponentSnapshot(component.getLocalBounds(), true, 1.0f);

            juce::PNGImageFormat png;
            out.deleteFile();
            juce::FileOutputStream stream(out);
            if (! stream.openedOk() || ! png.writeImageToStream(image, stream))
            {
                std::fprintf(stderr, "could not write %s\n", out.getFullPathName().toRawUTF8());
                result = 4;
            }

            std::printf("%s\n", component.describeLayout().toRawUTF8());
        }

        setApplicationReturnValue(result);
        quit();
    }

    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(ChordFoundryApplication)
