namespace Loudness {

/**
 * @brief Static utility for calculating raw loudness from FFT magnitude data.
 *
 * This class provides the core loudness calculation used by the Analyser.
 * It takes FFT magnitude bins and computes an average level value.
 *
 * ## Usage in Processing Chain
 *
 * Called by Analyser::calculateLoudness() after FFT processing:
 * ```
 * FFT Magnitude Bins → Loudness::Calculate() → Raw Loudness Value
 * ```
 *
 * ## Algorithm
 *
 * 1. For each FFT bin in the selected frequency range:
 *    - Map the magnitude from [0, 25] to [0, 1] (linear scaling)
 * 2. Return the average of all mapped values
 *
 * @note The 0-25 input range is empirically determined based on typical FFT
 *       output magnitudes. The dB parameters are currently unused but reserved
 *       for potential future perceptual loudness weighting.
 */
class Loudness {
public:
    /**
     * @brief Calculate average loudness from FFT magnitude data.
     *
     * @param data      Pointer to FFT magnitude values for selected frequency band
     * @param dataSize  Number of FFT bins to process
     * @param mindB     (Reserved) Minimum dB threshold
     * @param maxdB     (Reserved) Maximum dB threshold
     * @param fftSizeInDB (Reserved) FFT size in dB for normalization
     * @return Average loudness value, typically in range [0, 1]
     */
    static float Calculate(
        float* data, int dataSize, float mindB, float maxdB, float fftSizeInDB) {
        float average = 0.0F;

        // Map each FFT bin magnitude to a linear 0-1 scale and accumulate.
        // The goal is perceptual linearity where 0.5 feels approximately
        // half as loud as 1.0.
        for (int i = 0; i < dataSize; ++i) {
            auto gain = data[i];
            auto level = jmap(gain, 0.0F, 25.0F, 0.0F, 1.0F);
            average += level;
        }

        return average / static_cast<float>(dataSize);
    }
};
} // namespace Loudness
