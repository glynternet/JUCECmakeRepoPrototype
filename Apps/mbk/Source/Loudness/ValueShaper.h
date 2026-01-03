#include "JuceHeader.h"

namespace Loudness {

/**
 * @brief Maps input values from one range to another with clamping.
 *
 * Used in the loudness processing chain to map raw loudness values (which may
 * vary based on audio content) to a normalized 0-1 output range. This allows
 * calibration for different audio sources with varying dynamic ranges.
 *
 * ## Usage in Processing Chain
 *
 * The ValueShaper sits after raw loudness calculation and before smoothing:
 * ```
 * Raw Loudness [~0.0-1.0+] → ValueShaper → Normalized [0.0-1.0]
 * ```
 *
 * ## Sensitivity Control
 *
 * - **inMin**: Values at or below this become 0.0 (noise floor)
 * - **inMax**: Values at or above this become 1.0 (peak sensitivity)
 * - Narrower range = more sensitive to small changes
 * - Wider range = less sensitive, captures larger dynamic range
 *
 * Default range [0.1, 0.8] maps quiet passages near 0 and loud passages near 1.
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

    /**
     * @brief Map input value to output range with clamping to [0, 1].
     * @param value Input value to shape
     * @return Mapped and clamped output value
     */
    [[nodiscard]] float shape(float value) const {
        // TODO: allow configuration of this from the GUI in some advanced settings
        return jlimit(0.0f, 1.0f, jmap(value, _inMin, _inMax, _outMin, _outMax));
    }

private:
    float _inMin, _inMax;
    float _outMin, _outMax;
};
} // namespace Loudness
