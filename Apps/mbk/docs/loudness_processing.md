# Loudness Processing Chain

This document describes the audio-to-loudness processing chain in mbk that takes audio input and produces a stream of loudness values output via OSC.

## Overview

The loudness processing chain analyzes incoming audio in real-time and produces normalized loudness values (0.0 to 1.0) that can be used for visualization or external control via OSC messages.

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        LOUDNESS PROCESSING CHAIN                        │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│   Audio Input                                                           │
│   (File or Device)                                                      │
│        │                                                                │
│        ▼                                                                │
│   ┌─────────────────────────────────────────┐                          │
│   │        Sample Buffering (FIFO)          │   256 samples            │
│   │   Accumulates samples from audio thread │                          │
│   └─────────────────────────────────────────┘                          │
│        │                                                                │
│        ▼                                                                │
│   ┌─────────────────────────────────────────┐                          │
│   │          FFT Transform                   │   Hann window           │
│   │   256-point frequency-only FFT          │   @ 50 Hz (default)     │
│   └─────────────────────────────────────────┘                          │
│        │                                                                │
│        ▼                                                                │
│   ┌─────────────────────────────────────────┐                          │
│   │       Frequency Band Selection          │   2-13% of Nyquist      │
│   │   Extract magnitude for target range    │   (configurable)        │
│   └─────────────────────────────────────────┘                          │
│        │                                                                │
│        ▼                                                                │
│   ┌─────────────────────────────────────────┐                          │
│   │        Loudness Calculation             │                          │
│   │   Average magnitude → linear level      │                          │
│   └─────────────────────────────────────────┘                          │
│        │                                                                │
│        ▼                                                                │
│   ┌─────────────────────────────────────────┐                          │
│   │         Value Shaping                   │   [0.1, 0.8] → [0, 1]   │
│   │   Map to normalized output range        │   (configurable)        │
│   └─────────────────────────────────────────┘                          │
│        │                                                                │
│        ▼                                                                │
│   ┌─────────────────────────────────────────┐                          │
│   │        Moving Average                   │   Window: 2 samples      │
│   │   Smooth out rapid fluctuations         │   (configurable 1-7)    │
│   └─────────────────────────────────────────┘                          │
│        │                                                                │
│        ▼                                                                │
│   ┌─────────────────────────────────────────┐                          │
│   │          Tail-Off (Decay)               │   Coefficient: 0.8      │
│   │   Gradual decay, instant attack         │   (configurable)        │
│   └─────────────────────────────────────────┘                          │
│        │                                                                │
│        ▼                                                                │
│   Loudness Output [0.0 - 1.0]                                          │
│        │                                                                │
│        ├──────────────────────────┐                                    │
│        ▼                          ▼                                    │
│   ┌──────────────┐        ┌──────────────┐                             │
│   │ Visualization│        │  OSC Output  │                             │
│   │   (Graph)    │        │  /audio      │                             │
│   └──────────────┘        │  "loudness"  │                             │
│                           │   <float>    │                             │
│                           └──────────────┘                             │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## User Guide

### UI Controls

The loudness analyser provides several adjustable parameters via sliders:

| Control | Range | Default | Purpose |
|---------|-------|---------|---------|
| **Frequency Band** | 0.0 - 1.0 (dual) | 0.02 - 0.13 | Which frequencies contribute to loudness |
| **Range In** | -0.1 - 1.1 (dual) | 0.1 - 0.8 | Sensitivity calibration |
| **Decay Length** | 0.0 - 0.9999 | 0.8 | How quickly loudness drops |
| **Window Size** | 1 - 7 | 2 | Smoothing amount |
| **Process Rate** | 5 - 70 Hz | 50 | Analysis frequency |

### Parameter Guide

#### Frequency Band (Low/High)

Controls which frequency range is analyzed. Values are proportions of the Nyquist frequency (half the sample rate).

- **Default (0.02 - 0.13)**: Focuses on bass and low-mid frequencies
- **Wider range**: More frequencies contribute, general loudness
- **Narrower range**: Focus on specific frequency band

For 44.1kHz audio:
- 0.02 ≈ 440 Hz
- 0.13 ≈ 2,870 Hz

#### Range In (Min/Max)

Calibrates sensitivity to match your audio source. Maps raw loudness to 0-1 output.

- **Narrow range (0.3 - 0.5)**: High sensitivity, small changes = big output changes
- **Wide range (0.0 - 1.0)**: Low sensitivity, captures full dynamic range
- **Shift both values**: Adjust for quiet or loud sources

If output is always near 0: Lower the Min value
If output is always near 1: Raise the Max value

#### Decay Length

Controls how quickly the output drops after loud passages.

- **0.0**: No decay, output follows input exactly (jittery)
- **0.5**: Fast decay, responsive
- **0.8** (default): Moderate decay, smooth tail-off
- **0.95+**: Slow decay, values "stick" longer

Higher values create smoother visual feedback but slower response to quiet passages.

#### Window Size

Number of samples to average for smoothing.

- **1**: No smoothing (most responsive, most jittery)
- **2** (default): Minimal smoothing
- **5-7**: Heavy smoothing (very smooth, slower response)

#### Process Rate

How many times per second the FFT analysis runs.

- **Lower (5-20 Hz)**: Less CPU usage, choppier output
- **Default (50 Hz)**: Good balance
- **Higher (60-70 Hz)**: Smoother, more CPU usage

### OSC Output

Loudness values are sent via OSC to the configured IP address:

```
Address: /audio
Arguments: "loudness" (string), <value> (float32)
```

The float value ranges from 0.0 (silent) to 1.0 (maximum loudness after processing).

### Typical Use Cases

#### VJ/Lighting Control
- Keep default frequency band for bass response
- Set decay to 0.7-0.9 for smooth light transitions
- Adjust Range In to match your music's dynamic range

#### Audio Visualization
- Lower decay (0.3-0.5) for more reactive visuals
- Smaller window size (1-2) for responsiveness
- Higher process rate (60-70 Hz) for smooth animation

#### Ambient Monitoring
- Higher decay (0.9+) for stable readings
- Larger window size (5-7) for very smooth output
- Lower process rate to reduce CPU

## Code Architecture

### File Structure

```
Apps/mbk/Source/Loudness/
├── Analyser.h/.cpp         # Core FFT-based analysis engine
├── AnalyserComponent.h     # UI wrapper and integration
├── AnalyserSettings.h      # Parameter control sliders
├── Loudness.h              # Static loudness calculation utility
├── ValueShaper.h           # Input-to-output range mapping
├── MovingAverage.h         # Smoothing filter
├── TailOff.h/.cpp          # Decay effect
└── ValueHistoryComponent.h # Real-time visualization
```

### Thread Model

The processing uses two threads with atomic synchronization:

```
┌──────────────────────────────────────────────────────────────────┐
│                         AUDIO THREAD                              │
│  (runs at sample rate, e.g., 44,100 Hz)                          │
│                                                                   │
│  MainComponent::getNextAudioBlock()                              │
│       │                                                          │
│       ▼                                                          │
│  analyserComponent.pushNextSampleIntoFifo(sample)                │
│       │                                                          │
│       ▼                                                          │
│  Samples accumulate in 256-sample FIFO buffer                    │
│       │                                                          │
│       ▼                                                          │
│  When full: Copy to FFT buffer, set atomic flag                  │
│                           │                                       │
│                           ▼                                       │
│               nextFFTBlockReady = true  ─────────────────┐       │
│                                                           │       │
└───────────────────────────────────────────────────────────│───────┘
                                                            │
                    ┌───────────────────────────────────────┘
                    │
┌───────────────────▼──────────────────────────────────────────────┐
│                         TIMER THREAD                              │
│  (runs at process rate, default 50 Hz)                           │
│                                                                   │
│  if (nextFFTBlockReady) {                                        │
│      Apply Hann window                                           │
│      Perform FFT                                                 │
│      Calculate level through processing chain                    │
│      Invoke onLoudnessResult callback                            │
│      nextFFTBlockReady = false                                   │
│  }                                                                │
│                                                                   │
└──────────────────────────────────────────────────────────────────┘
```

### Processing Chain Implementation

Each stage is encapsulated in its own class:

1. **Analyser** (`Analyser.h`): Orchestrates the entire pipeline
   - Manages FIFO buffer and FFT processing
   - Coordinates between audio and timer threads
   - Calls each processing stage in sequence

2. **Loudness::Calculate** (`Loudness.h`): Raw level calculation
   - Takes FFT magnitude bins
   - Returns average level (0-1 range)

3. **ValueShaper** (`ValueShaper.h`): Range mapping
   - Maps input range to output range
   - Clamps output to [0, 1]

4. **MovingAverage** (`MovingAverage.h`): Smoothing
   - Circular buffer of recent values
   - Returns average of last N samples

5. **TailOff** (`TailOff.h`): Decay effect
   - Prevents abrupt drops
   - Output = max(input, previous * coefficient)

### Integration Points

**Entry Point** (audio input → analyser):
```cpp
// MainComponent.cpp:84
for (double sample: frameValues) {
    analyserComponent.pushNextSampleIntoFifo(static_cast<float>(sample));
}
```

**Exit Point** (analyser → OSC output):
```cpp
// MainComponent.cpp:7
analyserComponent([this](float level) { return oscSender.sendLoudness(level); })
```

### Adding Custom Processing

To add a new processing stage:

1. Create a new class in `Loudness/` following the pattern of existing stages
2. Add an instance to `Analyser.h`
3. Call it in `Analyser::calculateLevel()` at the appropriate point in the chain
4. Optionally add UI controls in `AnalyserSettings.h`

Example insertion point in `Analyser.cpp`:
```cpp
float Analyser::calculateLevel() {
    // ... existing code ...
    level = valueShaper.shape(level);
    // INSERT NEW PROCESSING HERE
    // level = yourNewProcessor.process(level);
    movingAverage.add(level);
    // ... rest of chain ...
}
```
