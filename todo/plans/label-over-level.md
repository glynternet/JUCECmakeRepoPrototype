# Plan: Label Over Level

## Summary

Position the label in ValueHistoryComponent to render on top of the history visualization rather than in a separate area. The gradient used for the level visualization makes it transparent enough for the label to remain visible.

## High-Level Design

### Current Behavior

The ValueHistoryComponent has:
- A `historySizeLabel` that says "History Size"
- A `historySizeSlider` for controlling history buffer size
- Both are positioned in the top area of the component (in non-compact mode)
- The label is attached to the slider via `attachToComponent()`

The label currently appears above/beside the slider, not overlaying the visualization.

### Target Behavior

Position the label (or a new dedicated label showing relevant information) directly over the visualization area. The gradient transparency ensures readability:

```
┌─────────────────────────────────┐
│                                 │
│         [Label Text]            │  <- Label overlays the visualization
│     ____    ____                │
│    /####\  /####\               │  <- Level visualization (gradient makes it semi-transparent)
│---/######\/######\---           │
│   \######/\######/              │
│    \____/  \____/               │
│                                 │
└─────────────────────────────────┘
```

## Low-Level Implementation

### Interpretation 1: Overlay History Size Label

Simply reposition the existing `historySizeLabel` to render over the visualization.

```cpp
void resized() override {
    pathNeedsRebuild = true;

    if (!isCompact) {
        // Position label in the center-left of the component
        const int labelWidth = 80;
        const int labelHeight = 20;
        historySizeLabel.setBounds(10, (getHeight() - labelHeight) / 2, labelWidth, labelHeight);

        // Position slider in top-right area
        const int sliderLeft = proportionOfWidth(0.69f);
        historySizeSlider.setBounds(sliderLeft, 10, getWidth() - sliderLeft - 10, 20);

        // Detach label from slider so it doesn't auto-position
        historySizeLabel.attachToComponent(nullptr, false);
    }
}
```

### Interpretation 2: Add Component Title Label (Recommended)

Add a new label that shows the component's purpose or current value, positioned prominently over the visualization.

```cpp
// In class definition
private:
    juce::Label titleLabel;

// In constructor
titleLabel.setText("Level", juce::dontSendNotification);
titleLabel.setFont(juce::Font(16.0f, juce::Font::bold));
titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
titleLabel.setJustificationType(juce::Justification::centred);
addAndMakeVisible(titleLabel);

// In resized()
void resized() override {
    pathNeedsRebuild = true;

    // Title label overlays the center-top of the visualization
    const int labelHeight = 24;
    const int labelMargin = 10;
    titleLabel.setBounds(labelMargin, labelMargin,
                         getWidth() - 2 * labelMargin, labelHeight);

    if (!isCompact) {
        const int sliderLeft = proportionOfWidth(0.69f);
        historySizeSlider.setBounds(sliderLeft, labelHeight + 15,
                                    getWidth() - sliderLeft - 10, 20);
    }
}
```

### Interpretation 3: Dynamic Value Label

Show the current (latest) level value as an overlay:

```cpp
// In class definition
private:
    juce::Label valueLabel;

// In constructor
valueLabel.setFont(juce::Font(14.0f));
valueLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
valueLabel.setJustificationType(juce::Justification::centredRight);
addAndMakeVisible(valueLabel);

// In addLevel()
void addLevel(float level) {
    ++latestValueIndex;
    latestValueIndex %= historySize;
    levelHistory[latestValueIndex] = level;
    pathNeedsRebuild = true;

    // Update value label
    valueLabel.setText(juce::String(level, 2), juce::dontSendNotification);

    repaint();
}

// In resized()
void resized() override {
    // ... existing code ...

    // Position value label in top-right, overlaying visualization
    const int labelWidth = 50;
    const int labelHeight = 20;
    valueLabel.setBounds(getWidth() - labelWidth - 10, 10, labelWidth, labelHeight);
}
```

### Paint Order Consideration

JUCE paints child components after the parent's `paint()` method by default. This means labels will automatically render on top of the visualization drawn in `paint()`. No z-order changes needed.

```cpp
// Paint order (automatic):
// 1. ValueHistoryComponent::paint() draws background and level paths
// 2. Child components (labels, sliders) paint on top
```

## API Changes

### Option: Configurable Label Text

```cpp
/** Set the overlay label text. Pass empty string to hide. */
void setLabelText(const juce::String& text);

/** Set the label visibility */
void setLabelVisible(bool visible);
```

### For PipelineViewComponent Integration

If using Interpretation 2 or 3, the PipelineViewComponent already has its own labels ("Raw", "Shaped", etc.). Consider:

```cpp
// In ValueHistoryComponent constructor, add parameter:
explicit ValueHistoryComponent(const juce::String& label = "")
    : labelText(label) {
    if (label.isNotEmpty()) {
        titleLabel.setText(label, juce::dontSendNotification);
        addAndMakeVisible(titleLabel);
    }
    // ... rest of constructor
}
```

Then PipelineViewComponent can pass labels:
```cpp
// In PipelineViewComponent
stages.at(i) = ValueHistoryComponent(stageNames.at(i));
```

## System Actor Interactions

### AnalyserComponent
- No changes required if using default label or no label
- Could optionally set a custom label like "Loudness"

### PipelineViewComponent
- If labels are integrated into ValueHistoryComponent, could remove the separate `labels` array
- Or keep external labels and disable internal labels via `setLabelVisible(false)`

## Performance Analysis

### Pros

1. **No Rendering Overhead**: Labels are lightweight JUCE components
2. **GPU Text Caching**: JUCE caches glyph rendering
3. **Automatic Layering**: No manual z-order management needed
4. **Visual Clarity**: Gradient transparency ensures label readability

### Cons

1. **Additional Component**: One more child component per ValueHistoryComponent
2. **String Updates**: If showing dynamic values, string formatting on each update

### Performance Impact

**Negligible.** Label rendering is highly optimized in JUCE. Even with 6 ValueHistoryComponents (1 main + 5 pipeline), the overhead of 6 additional label components is imperceptible.

For dynamic value labels, the `String::String(float, int)` formatting is the only potential concern, but this is called at most ~170 times/second (44100 Hz / 256 samples) and string formatting is fast for short numbers.

## Questions and Answers

**Q: Which interpretation is intended - repositioning existing labels or adding new overlay labels?**
A: Based on "The label in ValueHistoryComponent can be shown on top of the actual history line(s)", this suggests showing a label (title or value) overlaid on the visualization. **Interpretation 2 (Title Label)** is recommended as it provides useful context without the visual noise of constantly updating values.

**Q: Should the label be visible in compact mode?**
A: In compact mode, space is limited (used in PipelineViewComponent). The label could be:
- Hidden in compact mode (recommended - PipelineViewComponent has its own labels)
- Shown but smaller
- Controlled via a separate parameter

**Q: What should the default label text be?**
A: For the main AnalyserComponent usage, "Level" or "Loudness" would be appropriate. For PipelineViewComponent, each stage could use its name or no internal label.

**Q: Does label transparency need adjustment?**
A: Start with solid white (`Colours::white`). If readability is poor over the visualization, use `Colours::white.withAlpha(0.9f)` or add a subtle shadow/outline.

**Q: Should we add text shadow/outline for better contrast?**
A: Not initially. The gradient makes the visualization semi-transparent on the left side where the label would typically be positioned. If contrast is insufficient after implementation, add:
```cpp
// In paint(), before child components render:
g.setColour(Colours::black.withAlpha(0.3f));
g.fillRect(titleLabel.getBounds().expanded(2));
```
