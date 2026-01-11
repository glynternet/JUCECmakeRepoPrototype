#include "JuceHeader.h"
#include "PipelineStage.h"

namespace Loudness {

/**
 * @brief Maps input values from one range to another.
 *
 * Used in the loudness processing chain to map raw loudness values (which may
 * vary based on audio content) to a configurable output range. This allows
 * calibration for different audio sources with varying dynamic ranges.
 *
 * Note: This class performs linear mapping only. Final clamping to [0, 1]
 * happens at the end of the processing chain in Analyser::calculateLevel().
 *
 * ## Usage in Processing Chain
 *
 * The ValueShaper sits after raw loudness calculation and before smoothing:
 * ```
 * Raw Loudness [inMin, inMax] → ValueShaper → Output [outMin, outMax]
 * ```
 *
 * ## Range Control
 *
 * - **inMin/inMax**: Observed input range (learned from content or set manually)
 * - **outMin/outMax**: Target output range (user-configurable)
 *
 * With auto-ranging enabled, the input range is learned from actual audio
 * content while the output range is set by the user.
 */
class ValueShaper : public PipelineStage {
public:
    /**
     * @param input  Input range (values at start map to output start)
     * @param output Output range (typically 0.0 to 1.0)
     */
    ValueShaper(juce::Range<float> input, juce::Range<float> output)
        : _input(input)
        , _output(output) {}
    ~ValueShaper() = default;

    /** Set the input range */
    void setInputRange(juce::Range<float> range) { _input = range; }

    /** Set the output range */
    void setOutputRange(juce::Range<float> range) { _output = range; }

    /**
     * @brief Map input value to output range (linear interpolation).
     * @param value Input value to shape
     * @return Mapped output value (may exceed output range if input is outside input range)
     */
    [[nodiscard]] float shape(float value) const {
        return jmap(value, _input.getStart(), _input.getEnd(), _output.getStart(), _output.getEnd());
    }

    // PipelineStage interface
    float process(float input) override { return shape(input); }
    [[nodiscard]] const char* name() const override { return "Shaped"; }

private:
    juce::Range<float> _input;
    juce::Range<float> _output;
};
} // namespace Loudness
