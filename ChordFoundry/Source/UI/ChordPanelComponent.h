#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ModernLookAndFeel.h"

namespace ChordFoundry {

/**
 * ChordPanelComponent - Modern chord selection interface with pie slice wheel design
 * 
 * Features:
 * - Pie slice arrangement for intuitive chord selection
 * - Color-coded slices by function (primary, secondary, diminished)
 * - Smooth hover and selection animations
 * - Keyboard navigation support
 * - Accessibility labels and descriptions
 */
class ChordPanelComponent : public juce::Component,
                           public juce::Button::Listener
{
public:
    ChordPanelComponent();
    ~ChordPanelComponent() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Mouse handling for pie slices
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    
    // Button listener (for Add and Randomize buttons only)
    void buttonClicked(juce::Button* button) override;
    
    // Public API
    void setSelectedChord(const juce::String& roman);
    juce::String getSelectedChord() const { return selectedRoman; }
    
    // Callbacks
    std::function<void(const juce::String&)> onChordSelected;
    std::function<void()> onAddChord;
    
private:
    //==============================================================================
    // Chord data
    struct ChordInfo
    {
        juce::String roman;
        juce::String fullName;
        juce::Colour colour;
        bool isPrimary = false;
    };
    
    std::vector<ChordInfo> chordData;
    
    juce::String selectedRoman;
    int hoveredSliceIndex = -1;  // -1 means no hover
    int selectedSliceIndex = -1;  // -1 means no selection
    
    // UI elements
    std::unique_ptr<juce::Label> headerLabel;
    std::unique_ptr<juce::TextButton> addButton;
    std::unique_ptr<juce::TextButton> randomizeButton;
    
    // Layout
    static constexpr int WHEEL_SIZE = 320;
    static constexpr int CENTER_CIRCLE_SIZE = 100;
    static constexpr int WHEEL_THICKNESS = 90;  // Thickness of the pie slice ring
    
    //==============================================================================
    void setupChordData();
    void setupUI();
    void updateChordSelection();
    
    // Pie slice methods
    void drawPieSlice(juce::Graphics& g, int sliceIndex, float startAngle, float endAngle,
                      float innerRadius, float outerRadius, const juce::Point<float>& center);
    int getSliceIndexAt(const juce::Point<int>& point) const;
    float getAngleForSlice(int index) const;
    juce::Path createPieSlicePath(float startAngle, float endAngle, 
                                  float innerRadius, float outerRadius,
                                  const juce::Point<float>& center) const;
    
    juce::String getChordFullName(const juce::String& roman) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordPanelComponent)
};

} // namespace ChordFoundry