#include "JuceHeader.h"

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
class ValueShaper {
public:
    /**
     * @param inMin  Minimum input value (maps to outMin)
     * @param inMax  Maximum input value (maps to outMax)
     * @param outMin Minimum output value (typically 0.0)
     * @param outMax Maximum output value (typically 1.0)
     */
    ValueShaper(float inMin, float inMax, float outMin, float outMax)
        : _inMin(inMin)
        , _inMax(inMax)
        , _outMin(outMin)
        , _outMax(outMax) {}
    ~ValueShaper() = default;

    /** Set the input minimum (values below this map to outMin) */
    void setInMin(float value) { _inMin = value; }

    /** Set the input maximum (values above this map to outMax) */
    void setInMax(float value) { _inMax = value; }

    /** Set the output minimum */
    void setOutMin(float value) { _outMin = value; }

    /** Set the output maximum */
    void setOutMax(float value) { _outMax = value; }

    /**
     * @brief Map input value to output range (linear interpolation).
     * @param value Input value to shape
     * @return Mapped output value (may exceed [outMin, outMax] if input is outside [inMin, inMax])
     */
    [[nodiscard]] float shape(float value) const {
        return jmap(value, _inMin, _inMax, _outMin, _outMax);
    }

private:
    float _inMin, _inMax;
    float _outMin, _outMax;
};
} // namespace Loudness
