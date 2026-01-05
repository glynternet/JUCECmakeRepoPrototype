#pragma once

#include <algorithm>
#include <cmath>

namespace Loudness {

/**
 * @brief Loudness calculation modes.
 *
 * Controls how FFT magnitude data is converted to a loudness value.
 *
 * - **Linear**: Direct linear mapping of amplitude (legacy behavior).
 *   Fast but NOT perceptually accurate. Output 0.5 does NOT feel like
 *   half of 1.0.
 *
 * - **Perceptual**: Stevens' Power Law calculation. Works in power domain
 *   (amplitude squared) and applies Stevens exponent. Output 0.5 FEELS
 *   approximately half as loud as 1.0.
 */
enum class MappingMode {
    Linear,     ///< Legacy linear amplitude mapping
    Perceptual  ///< Stevens' Power Law (power domain, exponent 0.3)
};

/**
 * @brief Static utility for calculating loudness from FFT magnitude data.
 *
 * This class provides both linear and perceptual loudness calculations
 * for use in the audio analysis chain.
 *
 * ## Processing Chain Position
 *
 * Called by Analyser::calculateLoudness() after FFT and optional A-weighting:
 * ```
 * FFT Bins → [A-Weighting] → LoudnessCalculator::Calculate*() → Raw Value
 * ```
 *
 * ## Calculation Modes
 *
 * - **Linear (legacy)**: Simple linear mapping from [0, 25] to [0, 1].
 *   Does NOT achieve perceptual linearity despite the original goal.
 *
 * - **Perceptual (recommended)**: Implements Stevens' Power Law for true
 *   perceptual linearity where 0.5 feels approximately half as loud as 1.0.
 *
 * ## Stevens' Power Law
 *
 * S.S. Stevens (1957) discovered that perceived loudness L relates to
 * physical intensity I by:
 *
 *     L = k * I^0.3
 *
 * Since acoustic intensity is proportional to amplitude squared:
 *
 *     L ∝ amplitude^0.6
 *
 * Practical implications:
 * - 10x intensity increase = 2x perceived loudness
 * - To feel "half as loud", reduce to ~10% intensity (not 50%)
 * - Linear scaling does NOT achieve perceptual linearity
 *
 * ## Why Power Domain?
 *
 * We sum amplitude² (power) across bins before applying Stevens because:
 * 1. Acoustic intensity = pressure² (physical definition)
 * 2. Multiple sources combine by power addition (not amplitude)
 * 3. Averaging power, then applying Stevens, gives correct perception
 *
 * ## Relationship to A-Weighting
 *
 * A-weighting and Stevens' mapping serve complementary purposes:
 * - A-weighting: Corrects for frequency sensitivity (which Hz to count)
 * - Stevens' mapping: Corrects for amplitude-to-perception nonlinearity
 *
 * Both are needed for perceptually accurate loudness. Apply A-weighting
 * first (to FFT bins), then calculate perceptual loudness.
 *
 * @see AWeighting for frequency weighting implementation
 * @see Analyser for the full processing chain orchestration
 */
class LoudnessCalculator {
public:
    /**
     * @brief Stevens' Power Law exponent for intensity (power domain).
     *
     * The canonical value from psychoacoustic research. When applied to
     * normalized power (amplitude²), produces perceptually linear output.
     *
     * - 0.3 for intensity/power
     * - Equivalent to 0.6 for amplitude (since intensity = amplitude²)
     */
    static constexpr float kStevensExponent = 0.3F;

    /**
     * @brief Reference maximum power for normalization.
     *
     * Derived from the empirical maximum amplitude of 25.0:
     * kReferencePower = 25² = 625
     *
     * FFT magnitudes at or above 25.0 will produce output 1.0.
     */
    static constexpr float kReferencePower = 625.0F;

    /**
     * @brief Calculate perceptual loudness using Stevens' Power Law.
     *
     * This is the recommended calculation method for achieving true
     * perceptual linearity where output 0.5 feels approximately half
     * as loud as output 1.0.
     *
     * ## Algorithm
     *
     * 1. Square each magnitude to get power (intensity ∝ amplitude²)
     * 2. Average power across all bins
     * 3. Normalize to [0, 1] using kReferencePower (625)
     * 4. Apply Stevens' Power Law: perceived = power^0.3
     *
     * ## Example Values
     *
     * | Amplitude | Power | Normalized | Perceived |
     * |-----------|-------|------------|-----------|
     * | 0         | 0     | 0.0        | 0.0       |
     * | 12.5      | 156   | 0.25       | 0.66      |
     * | 25        | 625   | 1.0        | 1.0       |
     *
     * Note that mid-amplitude (12.5) produces ~0.66 perceived loudness,
     * NOT 0.5 as linear mapping would give.
     *
     * @param data     Pointer to FFT magnitude values
     * @param dataSize Number of FFT bins to process
     * @return Perceptual loudness in range [0, 1]
     */
    static float CalculatePerceptual(const float* data, int dataSize) {
        if (dataSize <= 0 || data == nullptr) {
            return 0.0F;
        }

        // Step 1: Sum power (amplitude²) across bins
        // Power domain is used because acoustic intensity ∝ pressure²
        float sumPower = 0.0F;
        for (int i = 0; i < dataSize; ++i) {
            sumPower += data[i] * data[i];
        }

        // Step 2: Average power
        float avgPower = sumPower / static_cast<float>(dataSize);

        // Step 3: Normalize to [0, 1] using reference maximum
        // Clamp to prevent output > 1 for very loud signals
        float normalizedPower = std::min(avgPower / kReferencePower, 1.0F);

        // Step 4: Apply Stevens' Power Law
        // perceived = power^0.3 gives perceptually linear output
        return std::pow(normalizedPower, kStevensExponent);
    }

    /**
     * @brief Calculate average loudness from FFT magnitude data (linear mode).
     *
     * This is the legacy calculation that provides backward compatibility.
     * It maps each magnitude linearly from [0, 25] to [0, 1] and averages.
     *
     * @warning This does NOT achieve perceptual linearity. Output 0.5 does
     *          NOT feel like half of 1.0. Use CalculatePerceptual() instead.
     *
     * @param data      Pointer to FFT magnitude values for selected frequency band
     * @param dataSize  Number of FFT bins to process
     * @param mindB     (Reserved) Minimum dB threshold - currently unused
     * @param maxdB     (Reserved) Maximum dB threshold - currently unused
     * @param fftSizeInDB (Reserved) FFT size in dB - currently unused
     * @return Average loudness value, typically in range [0, 1]
     *
     * @deprecated Prefer CalculatePerceptual() for new code requiring
     *             perceptual accuracy.
     */
    static float Calculate(
        float* data, int dataSize, float mindB, float maxdB, float fftSizeInDB) {
        // Suppress unused parameter warnings - reserved for future use
        (void) mindB;
        (void) maxdB;
        (void) fftSizeInDB;

        float average = 0.0F;

        // Linear mapping: each bin scaled from [0, 25] to [0, 1]
        // NOTE: This is NOT perceptually linear. Human perception follows
        // Stevens' Power Law, not linear scaling. See CalculatePerceptual().
        for (int i = 0; i < dataSize; ++i) {
            auto gain = data[i];
            auto level = jmap(gain, 0.0F, 25.0F, 0.0F, 1.0F);
            average += level;
        }

        return average / static_cast<float>(dataSize);
    }
};

} // namespace Loudness
