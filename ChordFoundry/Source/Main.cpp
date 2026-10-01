#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <cstdio>
#include "MainComponent.h"
#include "UI/ChordModifierDialog.h"
#include "UI/PatternEditorComponent.h"

//==============================================================================
class ChordFoundryApplication : public juce::JUCEApplication
{
public:
    //==============================================================================
    ChordFoundryApplication() {}

    const juce::String getApplicationName() override { return "Chord Foundry"; }
    const juce::String getApplicationVersion() override { return "1.1.0"; }
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

        // Developer tool: send mouse events to the pattern editor (draw, move, resize, click) and
        // check the blocks it ends up with.  ChordFoundry --editor-selfcheck
        if (args.contains("--editor-selfcheck"))
        {
            runEditorSelfCheck();
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
    void runEditorSelfCheck()
    {
        using namespace ChordFoundry;

        int failures = 0;
        const auto check = [&failures](bool ok, const juce::String& what)
        {
            std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what.toRawUTF8());
            if (! ok)
                ++failures;
        };

        {
            // The model stands in for MainComponent: the editor reports each change to it, and
            // the editor is then shown what the model kept.
            ChordProgression model;
            model.addChord(ChordData("I"));
            model.addChord(ChordData("V"));

            PatternEditorComponent editor;
            editor.setSize(1168, 190);
            editor.setChordCount(2);

            const auto sync = [&] { editor.setBlocks(model.getBlocks()); };
            editor.onBlockAdded = [&](const BlockData& b) { model.addBlock(b); sync(); };
            editor.onBlockRemoved = [&](int i) { model.removeBlock(i); sync(); };
            editor.onBlockModified = [&](int i, const BlockData& b) { model.replaceBlock(i, b); sync(); };

            const auto event = [&](juce::Point<int> p, int modifiers, bool dragged)
            {
                const auto position = p.toFloat();
                return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), position,
                                        juce::ModifierKeys(modifiers), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                        &editor, &editor, juce::Time::getCurrentTime(), position,
                                        juce::Time::getCurrentTime(), 1, dragged);
            };
            const auto left = juce::ModifierKeys::leftButtonModifier;
            const auto centre = [&](int step) { return editor.getStepBounds(step).getCentre(); };
            const auto nearRightEdge = [&](int step)
            {
                const auto b = editor.getStepBounds(step);
                return juce::Point<int>(b.getRight() - 3, b.getCentreY());
            };
            const auto drag = [&](juce::Point<int> from, juce::Point<int> to, int modifiers = left)
            {
                editor.mouseDown(event(from, modifiers, false));
                editor.mouseDrag(event(to, modifiers, true));
                editor.mouseUp(event(to, modifiers, true));
            };
            const auto click = [&](juce::Point<int> at, int modifiers = left)
            {
                editor.mouseDown(event(at, modifiers, false));
                editor.mouseUp(event(at, modifiers, false));
            };
            const auto blockAt = [&](int start) -> const BlockData*
            {
                for (const auto& b : model.getBlocks())
                    if (b.startStep == start)
                        return &b;
                return nullptr;
            };

            // Draw: drag across empty steps 2..5
            drag(centre(2), centre(5));
            check(model.getBlockCount() == 1 && blockAt(2) != nullptr && blockAt(2)->lengthSteps == 4,
                  "drag across empty steps 2-5 draws a block of 4 steps");

            // Move: grab step 3 (second step of the block), drop on step 7
            drag(centre(3), centre(7));
            check(model.getBlockCount() == 1 && blockAt(6) != nullptr && blockAt(6)->lengthSteps == 4,
                  "dragging the block from step 3 to step 7 moves it to start at step 6");

            // Resize: drag the right edge of its last step (step 9) out to step 12
            drag(nearRightEdge(9), centre(12));
            check(blockAt(6) != nullptr && blockAt(6)->lengthSteps == 7, "dragging the right edge to step 12 resizes it to 7 steps");

            // Shrink with the same handle
            drag(nearRightEdge(12), centre(8));
            check(blockAt(6) != nullptr && blockAt(6)->lengthSteps == 3, "dragging the edge back to step 8 shrinks it to 3 steps");

            // A second block, then a move that would collide stays put
            drag(centre(20), centre(23));
            check(model.getBlockCount() == 2 && blockAt(20) != nullptr, "a second block is drawn at step 20");
            drag(centre(7), centre(21));
            check(blockAt(6) != nullptr && blockAt(20) != nullptr && model.getBlockCount() == 2,
                  "a move onto the other block is refused; both blocks keep their places");

            // Resizing is limited by the next block
            drag(nearRightEdge(8), centre(30));
            check(blockAt(6) != nullptr && blockAt(6)->lengthSteps == 14, "growing stops at the next block (step 20)");

            // A moved block cannot leave the grid
            drag(centre(21), centre(31));
            check(blockAt(28) != nullptr && blockAt(28)->lengthSteps == 4, "a block dragged past the end stops at the end of the grid");

            // Drawing across an existing block stops before it
            drag(centre(0), centre(10));
            check(blockAt(0) != nullptr && blockAt(0)->lengthSteps == 6, "drawing from 0 towards 10 stops at the block at 6");

            // Shift-click toggles strike
            click(centre(0), left | juce::ModifierKeys::shiftModifier);
            check(blockAt(0) != nullptr && blockAt(0)->newStrike, "Shift-click on a block's first step marks it to strike again");
            click(centre(0), left | juce::ModifierKeys::shiftModifier);
            check(blockAt(0) != nullptr && ! blockAt(0)->newStrike, "and again clears it");

            // Click removes; right-click removes; click on empty adds a one-step block
            const int before = model.getBlockCount();
            click(centre(2));
            check(model.getBlockCount() == before - 1 && blockAt(0) == nullptr, "a plain click on a block removes it");
            click(centre(30), juce::ModifierKeys::rightButtonModifier);
            check(model.getBlockCount() == before - 2, "right-click removes the block under the mouse");
            click(centre(25));
            check(blockAt(25) != nullptr && blockAt(25)->lengthSteps == 1, "a click on an empty step adds a one-step block");
        }

        std::printf("%d failure(s)\n", failures);
        setApplicationReturnValue(failures == 0 ? 0 : 1);
        quit();
    }

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

            ChordData modified = c.getChordForCheck(0);
            modified.extension = "+7th";
            modified.arpMode = "Up";
            c.onChordModified(0, modified);
            check(c.hasUnsavedChanges(), "editing a chord's modifiers marks the project changed");

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
            check(afterProgression.getChord(0).extension == "+7th" && afterProgression.getChord(0).arpMode == "Up",
                  "the chord modifiers were saved");
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

        // --dialog renders the chord modifier dialog (with Custom voicing open) instead.
        if (args.contains("--dialog"))
        {
            ChordFoundry::ChordData chord("vi");
            chord.extension = "+7th";
            chord.voicing = "Custom";
            chord.arpMode = "Up";
            chord.arpLength = "1/8";

            ChordFoundry::ChordModifierDialog dialog(chord);
            auto* content = dialog.getContentComponent();
            const auto image = content->createComponentSnapshot(content->getLocalBounds(), true, 1.0f);

            juce::PNGImageFormat png;
            out.deleteFile();
            juce::FileOutputStream stream(out);
            if (! stream.openedOk() || ! png.writeImageToStream(image, stream))
                result = 4;

            std::printf("dialog content %dx%d\n", content->getWidth(), content->getHeight());
            setApplicationReturnValue(result);
            quit();
            return;
        }

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
