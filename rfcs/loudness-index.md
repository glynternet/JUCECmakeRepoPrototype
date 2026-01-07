# Loudness Index Implementation Plan

## Overview

The **Loudness Index** is a longer-term statistical measure of processed loudness output that represents the "average activity level" of the audio-visual system. Unlike the instantaneous loudness value (which changes rapidly at ~60Hz), the Loudness Index provides stable metrics over meaningful time windows.

### Purpose

During an audio-visual show, operators need to:
1. **Observe** the system's behavior over time (current state)
2. **Target** a desired loudness profile (future state)
3. **Tune** the system to achieve targets automatically (automation)

The Loudness Index provides a simple abstraction (two numbers: Index and Range) that represents complex underlying parameters, enabling intuitive show control.

### Design Decisions

**Measurement Point:** After the final `jlimit(0, 1)` in `Analyser.cpp:118`
- Rationale: This is exactly what consumers receive via OSC
- The value is already normalized to [0.0, 1.0]
- All shaping, smoothing, and decay has been applied

**Scale:** 0-10 for human readability
- Internal calculation uses 0.0-1.0, multiplied by 10 for display
- Easier for operators to communicate ("Index at 6" vs "Index at 0.6")

**Time Windows:** Linux load-average style triple display (10s / 1m / 5m)
- 10 seconds: Immediate responsiveness - "what's happening now"
- 1 minute: Short-term trend - "this section's energy"
- 5 minutes: Overall vibe - "the show's average so far"

---

## Current Signal Flow

```
Audio -> FFT -> A-Weight -> Loudness Calc
                              |
                              v
                     +------------------+
                     |   RangeAdapter   |  Learns input range from content
                     +--------+---------+
                              |
                              v
                     +------------------+
                     |   ValueShaper    |  Maps to target output range
                     +--------+---------+
                              |
                              v
                     +------------------+
                     |    Smoother      |  EWMA smoothing (recently added)
                     +--------+---------+
                              |
                              v
                     +------------------+
                     |    TailOff       |  Decay/sticky effect
                     +--------+---------+
                              |
                              v
                     +------------------+
                     |  jlimit(0, 1)    |  Final clamp <-- LOUDNESS INDEX READS HERE
                     +--------+---------+
                              |
                              v
                          OSC Output
```

---

## Stage 1: AdaptiveRange Abstraction ✅

**Status:** Complete (commit 05d2c0d)

**Goal:** Extract reusable range-tracking logic from RangeAdapter.

### Rationale

The `RangeAdapter` contains sophisticated logic for tracking min/max bounds:
- Asymmetric adaptation (fast expansion, slow contraction)
- Smoothed signal tracking for intelligent contraction
- Minimum separation enforcement

This same logic is needed for tracking the Loudness Range (the spread of output values). Rather than duplicating code, we extract a generic `AdaptiveRange` class.

### Implementation

**New file:** `Apps/mbk/Source/Loudness/AdaptiveRange.h`

```cpp
namespace Loudness {

/**
 * @brief Generic adaptive min/max tracker with asymmetric rates.
 *
 * Tracks a value stream and learns the observed range over time.
 * Expands quickly when values exceed bounds, contracts slowly otherwise.
 *
 * Thread-safe via atomics for cross-thread access.
 */
class AdaptiveRange {
public:
    AdaptiveRange(float initialMin, float initialMax,
                  float adaptRate, float minSeparation = 0.05f);

    void update(float value) noexcept;

    [[nodiscard]] float getMin() const noexcept;
    [[nodiscard]] float getMax() const noexcept;
    [[nodiscard]] float getRange() const noexcept;   // max - min
    [[nodiscard]] float getCenter() const noexcept;  // (max + min) / 2

    void setEnabled(bool value) noexcept;
    void setLocked(bool value) noexcept;
    void setAdaptationRate(float rate) noexcept;
    void setBounds(float min, float max) noexcept;

private:
    std::atomic<float> observedMin;
    std::atomic<float> observedMax;
    std::atomic<float> alpha;
    std::atomic<float> smoothedSignal;
    std::atomic<bool> enabled{true};
    std::atomic<bool> locked{false};
    float minSeparation;
};

}  // namespace Loudness
```

**Refactor:** `RangeAdapter` to use `AdaptiveRange` internally

```cpp
class RangeAdapter {
    AdaptiveRange inputRange;  // Delegates to abstraction
    std::atomic<float> targetOutMin;
    std::atomic<float> targetOutMax;
    // ... existing interface unchanged
};
```

### Verification

- All existing tests pass
- No behavioral changes to existing functionality
- RangeAdapter API remains unchanged

### Pros
- DRY principle: shared logic in one place
- Easier to test the core algorithm in isolation
- LoudnessIndex can reuse without duplication

### Cons
- Additional abstraction layer
- Slightly more complex file structure

### Files Changed
- New: `AdaptiveRange.h`
- Modified: `RangeAdapter.h` (internal refactor only)
- New: `AdaptiveRangeTest.cpp` (optional but recommended)

---

## Stage 2: LoudnessIndex Core (Display Only) ✅

**Status:** Complete

**Goal:** Calculate and expose Loudness Index and Range metrics.

### Implementation

**New file:** `Apps/mbk/Source/Loudness/LoudnessIndex.h`

```cpp
namespace Loudness {

/**
 * @brief Tracks long-term loudness statistics (Index and Range).
 *
 * Provides Linux load-average style metrics at three time scales:
 * - 10 seconds: Immediate responsiveness
 * - 1 minute: Short-term trend
 * - 5 minutes: Overall average
 *
 * Also tracks the dynamic range of output values.
 */
class LoudnessIndex {
public:
    LoudnessIndex();

    /**
     * @brief Update with a new output level (call after jlimit).
     * @param level Final processed level [0.0, 1.0]
     */
    void update(float level) noexcept;

    // Index values (0-10 scale)
    [[nodiscard]] float getIndex10s() const noexcept;
    [[nodiscard]] float getIndex1m() const noexcept;
    [[nodiscard]] float getIndex5m() const noexcept;

    // Range value (0-10 scale)
    [[nodiscard]] float getRange() const noexcept;

private:
    // Three EWMA trackers for different time windows
    // At ~60 updates/sec: alpha = 1/samples
    std::atomic<float> ema10s{0.5f};   // alpha ~ 0.00167 (600 samples)
    std::atomic<float> ema1m{0.5f};    // alpha ~ 0.000278 (3600 samples)
    std::atomic<float> ema5m{0.5f};    // alpha ~ 0.0000556 (18000 samples)

    // Range tracking using AdaptiveRange
    AdaptiveRange outputRange;
};

}  // namespace Loudness
```

### Alpha Values for Time Windows

EWMA doesn't have a hard window, but we can calculate an "effective window" based on how many samples it takes for a value's influence to decay to ~37% (1/e):

```
effective_samples ~ 1/alpha
```

At approximately 60 updates/second (256-sample FFT @ 48kHz, with processing overhead):

| Window | Samples | Alpha Value |
|--------|---------|-------------|
| 10 sec | 600     | 0.00167     |
| 1 min  | 3600    | 0.000278    |
| 5 min  | 18000   | 0.0000556   |

### Integration Point

**File:** `Analyser.h` - Add member:
```cpp
LoudnessIndex loudnessIndex;
```

**File:** `Analyser.cpp:118` - After jlimit:
```cpp
float Analyser::calculateLevel() {
    // ... existing processing ...

    float finalLevel = level < 0.0001F ? 0.0F : jlimit(0.0F, 1.0F, level);
    loudnessIndex.update(finalLevel);  // NEW
    return finalLevel;
}
```

### Callback Extension

Extend the existing `onLoudnessResult` callback or add a separate callback to surface the index values:

```cpp
// Option A: Extend existing callback signature
std::function<void(float level, float index10s, float index1m, float index5m, float range)>

// Option B: Separate callback (less intrusive)
std::function<void(float index10s, float index1m, float index5m, float range)> onIndexUpdate;
```

Option B is recommended to avoid changing existing callback consumers.

### Verification
- Build succeeds
- Index values update in real-time
- Values stabilize appropriately at each time scale

### Pros
- Non-invasive addition to existing chain
- Reuses proven EWMA approach (same as Smoother)
- Thread-safe via atomics

### Cons
- Additional processing per sample (minimal: 4 atomic operations)
- Memory for 4 atomic floats + AdaptiveRange

### Files Changed
- New: `LoudnessIndex.h`
- Modified: `Analyser.h` (add member)
- Modified: `Analyser.cpp` (add update call)
- New: `LoudnessIndexTest.cpp`

---

## Stage 3: UI Display

**Goal:** Display Loudness Index and Range as read-only values in the UI.

### Design Principle

**Only show what exists.** No toggles, buttons, or controls for features not yet implemented. This stage adds read-only displays only.

### UI Layout

Add to `AnalyserSettings.h` below existing controls:

```
+-------------------------------------------------------------+
|  [Existing controls: frequency, ranges, toggles, etc.]      |
+-------------------------------------------------------------+
|                                                             |
|  Loudness Index    6.2 / 5.8 / 5.4                         |
|                    10s   1m    5m                           |
|                                                             |
|  Loudness Range    4.8                                      |
|                                                             |
+-------------------------------------------------------------+
```

### Implementation

```cpp
// In AnalyserSettings.h - add members:
juce::Label indexLabel;
juce::Label indexValue;
juce::Label rangeLabel;
juce::Label rangeValue;

// In constructor:
indexLabel.setText("Index", juce::dontSendNotification);
indexLabel.setJustificationType(juce::Justification::centredRight);
addAndMakeVisible(indexLabel);

indexValue.setText("- / - / -", juce::dontSendNotification);
indexValue.setJustificationType(juce::Justification::centredLeft);
addAndMakeVisible(indexValue);

rangeLabel.setText("Range", juce::dontSendNotification);
rangeLabel.setJustificationType(juce::Justification::centredRight);
addAndMakeVisible(rangeLabel);

rangeValue.setText("-", juce::dontSendNotification);
rangeValue.setJustificationType(juce::Justification::centredLeft);
addAndMakeVisible(rangeValue);

// In resized():
// ... layout the new labels ...

// Update method (called from message thread via callback):
void updateIndexDisplay(float i10s, float i1m, float i5m, float range) {
    auto indexText = juce::String::formatted("%.1f / %.1f / %.1f", i10s, i1m, i5m);
    indexValue.setText(indexText, juce::dontSendNotification);
    rangeValue.setText(juce::String(range, 1), juce::dontSendNotification);
}
```

### Thread Safety

The callback must marshal to the message thread before updating UI:

```cpp
// In AnalyserComponent.h callback:
MessageManager::callAsync([this, i10s, i1m, i5m, range]() {
    settings.updateIndexDisplay(i10s, i1m, i5m, range);
});
```

### Verification
- UI displays update in real-time
- Values are stable (not jumping wildly)
- 10s index responds faster than 1m, which responds faster than 5m
- Range reflects the actual spread of output values

### Pros
- Operators can observe system behavior
- No behavioral changes to processing
- Foundation for future automation

### Cons
- Additional UI complexity
- Must ensure thread-safe updates

### Files Changed
- Modified: `AnalyserSettings.h` (add labels)
- Modified: `AnalyserComponent.h` (wire up callback)

---

## Stage 4: Target Settings

**Goal:** Allow operators to set target Index and Range values, with visual feedback showing the delta from current values.

### Design

Add two sliders for targets, plus delta indicators:

```
+-------------------------------------------------------------+
|  Loudness Index    6.2 / 5.8 / 5.4                         |
|  Target Index      [====|========] 7.0    delta +0.8        |
|                                                             |
|  Loudness Range    4.8                                      |
|  Target Range      [======|======] 5.0    delta +0.2        |
+-------------------------------------------------------------+
```

The delta shows how far current values are from targets:
- Positive delta: Current is below target (need to increase)
- Negative delta: Current is above target (need to decrease)
- Near zero: On target

### Implementation

```cpp
// In AnalyserSettings.h - add members:
juce::Slider targetIndexSlider;
juce::Slider targetRangeSlider;
juce::Label targetIndexDelta;
juce::Label targetRangeDelta;

// Slider configuration:
targetIndexSlider.setRange(0.0, 10.0, 0.1);
targetIndexSlider.setValue(6.0);
targetIndexSlider.setSliderStyle(juce::Slider::LinearHorizontal);

targetRangeSlider.setRange(0.0, 10.0, 0.1);
targetRangeSlider.setValue(5.0);
targetRangeSlider.setSliderStyle(juce::Slider::LinearHorizontal);

// Update method extension:
void updateIndexDisplay(float i10s, float i1m, float i5m, float range) {
    // ... existing update ...

    // Calculate deltas (using 1m as reference for stability)
    float indexDelta = targetIndexSlider.getValue() - i1m;
    float rangeDelta = targetRangeSlider.getValue() - range;

    targetIndexDelta.setText(formatDelta(indexDelta), juce::dontSendNotification);
    targetRangeDelta.setText(formatDelta(rangeDelta), juce::dontSendNotification);
}

juce::String formatDelta(float delta) {
    if (std::abs(delta) < 0.1f) return "=";  // On target
    return juce::String::formatted("%+.1f", delta);
}
```

### Verification
- Targets can be adjusted via sliders
- Delta display updates in real-time
- Values are persisted (optional: save/restore with app state)

### Pros
- Operators can set goals
- Visual feedback guides manual tuning
- Foundation for automation

### Cons
- More UI controls
- Operators must still manually adjust other parameters

### Files Changed
- Modified: `AnalyserSettings.h` (add sliders and delta labels)

---

## Stage 5: Automatic Control (Feedback Loop)

**Goal:** Automatically adjust processing parameters to achieve target Index and Range.

### Control Strategy

The controller adjusts **output range bounds** (outMin, outMax) to achieve targets:

| Adjustment | Effect on Index | Effect on Range |
|------------|-----------------|-----------------|
| Raise outMax | Raises index | Increases range |
| Raise outMin | Raises index | Decreases range |
| Lower outMax | Lowers index | Decreases range |
| Lower outMin | Lowers index | Increases range |

**Resolution strategy when Index and Range targets conflict:**

Example: Index is 5.0 (target 7.0) and Range is 6.0 (target 4.0)
- To raise Index: increase outMax or outMin
- To lower Range: decrease spread (raise outMin, lower outMax)

Conflict: Raising outMax increases both Index AND Range.

**Solution:** Prioritize Index, then adjust Range by shifting both bounds:
1. Calculate required shift to achieve Index target
2. Apply shift to both outMin and outMax equally
3. Then adjust spread (outMax - outMin) to achieve Range target

### Implementation

**New file:** `Apps/mbk/Source/Loudness/LoudnessIndexController.h`

```cpp
namespace Loudness {

/**
 * @brief Feedback controller for automatic loudness index targeting.
 *
 * Uses PI control to adjust output range bounds to achieve target
 * Index and Range values.
 */
class LoudnessIndexController {
public:
    void setTargetIndex(float target) noexcept;
    void setTargetRange(float target) noexcept;
    void setEnabled(bool value) noexcept;

    /**
     * @brief Update controller with current measurements.
     *
     * Called periodically (e.g., every 10th processing cycle).
     * Modifies outMin/outMax to drive toward targets.
     *
     * @param currentIndex Current 1-minute index (most stable)
     * @param currentRange Current range measurement
     * @param[in,out] outMin Output range minimum to adjust
     * @param[in,out] outMax Output range maximum to adjust
     */
    void update(float currentIndex, float currentRange,
                float& outMin, float& outMax) noexcept;

private:
    std::atomic<float> targetIndex{6.0f};
    std::atomic<float> targetRange{5.0f};
    std::atomic<bool> enabled{false};

    // PI controller state
    float indexIntegral = 0.0f;
    float rangeIntegral = 0.0f;

    // Tuning parameters
    static constexpr float kp = 0.01f;   // Proportional gain
    static constexpr float ki = 0.001f;  // Integral gain
    static constexpr float deadband = 0.2f;  // Don't adjust if within this range
};

}  // namespace Loudness
```

### UI Addition

Add "Auto" toggle (only now that the feature exists):

```cpp
// In AnalyserSettings.h:
juce::ToggleButton autoIndexToggle;

autoIndexToggle.setButtonText("Auto");
autoIndexToggle.onStateChange = [this]() {
    bool enabled = autoIndexToggle.getToggleState();
    analyser.setAutoIndexEnabled(enabled);

    // Disable manual sliders when auto is enabled
    targetIndexSlider.setEnabled(!enabled);
    targetRangeSlider.setEnabled(!enabled);
};
```

### Integration

```cpp
// In Analyser.cpp - called periodically from processing thread:
if (autoIndexEnabled && ++controllerCounter >= 10) {
    controllerCounter = 0;

    float outMin = rangeAdapter.getTargetOutMin();
    float outMax = rangeAdapter.getTargetOutMax();

    indexController.update(
        loudnessIndex.getIndex1m(),
        loudnessIndex.getRange(),
        outMin, outMax
    );

    rangeAdapter.setTargetOutRange(outMin, outMax);
}
```

### Safety Constraints

The controller enforces bounds to prevent runaway behavior:
- outMin clamped to [0.0, 0.8]
- outMax clamped to [0.2, 1.0]
- Minimum spread enforced (outMax - outMin >= 0.1)
- Rate limiting to prevent rapid oscillation

### Verification
- Enable Auto and set targets
- Verify system converges toward targets
- Verify stability (no oscillation)
- Verify manual override disables automation
- Test edge cases (extreme targets, rapid audio changes)

### Pros
- Hands-off operation during shows
- Consistent behavior regardless of input material
- Enables timeline-based automation (future)

### Cons
- Complex control loop to tune
- Potential for oscillation if poorly tuned
- Operators may not understand what's being adjusted

### Files Changed
- New: `LoudnessIndexController.h`
- Modified: `Analyser.h` (add controller member)
- Modified: `Analyser.cpp` (integrate controller)
- Modified: `AnalyserSettings.h` (add Auto toggle)
- New: `LoudnessIndexControllerTest.cpp`

---

## Future Considerations

### Timeline Automation

Store target Index/Range curves as timelines:
```
Time 0:00  - Index: 6, Range: 5 (calm opening)
Time 0:30  - Index: 7, Range: 6 (building energy)
Time 1:00  - Index: 8, Range: 7 (peak intensity)
Time 1:30  - Index: 5, Range: 3 (intimate moment)
```

### Preset System

Save/recall named presets:
- "Opening Act"
- "Peak Energy"
- "Ambient Background"

### OSC Control

Expose targets via OSC for external automation:
- `/audio/target/index <float>`
- `/audio/target/range <float>`
- `/audio/auto <bool>`

### Multi-Band Index

Track separate indices for different frequency bands:
- Low (bass) index
- Mid index
- High (treble) index

---

## Risk Assessment

| Risk | Mitigation |
|------|------------|
| Controller oscillation | Rate limiting, deadband, integral windup protection |
| Thread safety issues | All cross-thread communication via atomics |
| UI responsiveness | Callback marshaling to message thread |
| Breaking existing behavior | Each stage is isolated and tested independently |
| Operator confusion | Clear labeling, documentation, gradual rollout |

---

## Testing Strategy

### Unit Tests
- `AdaptiveRangeTest.cpp`: Verify expansion/contraction behavior
- `LoudnessIndexTest.cpp`: Verify EWMA calculations at each time scale
- `LoudnessIndexControllerTest.cpp`: Verify convergence behavior

### Integration Tests
- Full chain with known input produces expected Index values
- Controller achieves targets within acceptable time
- No audio glitches during controller operation

### Manual Testing
- Real audio content produces sensible Index values
- UI updates are smooth and responsive
- Auto mode feels natural to operate

---

## Summary

| Stage | Deliverable | Complexity |
|-------|-------------|------------|
| 1 | AdaptiveRange abstraction | Low |
| 2 | LoudnessIndex core calculation | Medium |
| 3 | UI display (read-only) | Low |
| 4 | Target sliders with delta display | Low |
| 5 | Automatic feedback control | High |

Each stage is independently useful and testable. Stages 1-3 provide immediate value (observability) with minimal risk. Stages 4-5 add operator control and automation.
