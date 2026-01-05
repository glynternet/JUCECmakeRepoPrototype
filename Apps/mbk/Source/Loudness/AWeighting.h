#pragma once

#include <cmath>
#include <vector>

namespace Loudness {

/**
 * @brief A-weighting filter for perceptual loudness measurement.
 *
 * A-weighting approximates human hearing sensitivity across frequencies.
 * It attenuates low frequencies (<500 Hz) and high frequencies (>6 kHz)
 * where hearing is less sensitive, and emphasizes 2-4 kHz where hearing
 * is most sensitive.
 *
 * ## Usage
 *
 * 1. Call prepare() once when sample rate is known
 * 2. Call apply() on FFT magnitude bins before loudness calculation
 *
 * The weights are pre-computed for efficiency - not calculated per frame.
 *
 * ## Standard
 *
 * Implements IEC 61672:2003 A-weighting curve.
 */
class AWeighting {
public:
    /**
     * @brief Pre-compute A-weighting values for FFT bins.
     *
     * @param fftSize Total FFT size (e.g., 256)
     * @param sampleRate Audio sample rate in Hz (e.g., 44100)
     */
    void prepare(int fftSize, double sampleRate) {
        if (sampleRate == cachedSampleRate && fftSize == cachedFftSize) {
            return; // Already computed for these parameters
        }

        cachedSampleRate = sampleRate;
        cachedFftSize = fftSize;

        int numBins = fftSize / 2;
        weights.resize(static_cast<size_t>(numBins));

        for (int bin = 0; bin < numBins; ++bin) {
            double freq = binToFrequency(bin, fftSize, sampleRate);
            // Avoid division by zero at DC
            if (freq < 1.0) {
                freq = 1.0;
            }
            weights[static_cast<size_t>(bin)] = static_cast<float>(aWeightingGain(freq));
        }
    }

    /**
     * @brief Apply A-weighting to FFT magnitude bins.
     *
     * @param magnitudes Pointer to FFT magnitude data
     * @param numBins Number of bins to process
     * @param startBin Starting bin index (for correct frequency mapping)
     */
    void apply(float* magnitudes, int numBins, int startBin) const {
        for (int i = 0; i < numBins; ++i) {
            int globalBin = startBin + i;
            if (globalBin >= 0 && globalBin < static_cast<int>(weights.size())) {
                magnitudes[i] *= weights[static_cast<size_t>(globalBin)];
            }
        }
    }

    /**
     * @brief Get A-weighting gain for a specific frequency.
     *
     * @param hz Frequency in Hz
     * @return Linear gain multiplier (not dB)
     */
    [[nodiscard]] static double aWeightingGain(double hz) {
        // IEC 61672:2003 A-weighting formula
        // A(f) = 12194^2 * f^4 / ((f^2 + 20.6^2) * sqrt((f^2 + 107.7^2)(f^2 + 737.9^2)) * (f^2 + 12194^2))

        constexpr double f1 = 20.598997;
        constexpr double f2 = 107.65265;
        constexpr double f3 = 737.86223;
        constexpr double f4 = 12194.217;

        double f2_sq = hz * hz;
        double f1_sq = f1 * f1;
        double f2_const_sq = f2 * f2;
        double f3_sq = f3 * f3;
        double f4_sq = f4 * f4;

        double numerator = f4_sq * f2_sq * f2_sq;
        double denominator = (f2_sq + f1_sq) * std::sqrt((f2_sq + f2_const_sq) * (f2_sq + f3_sq)) *
                             (f2_sq + f4_sq);

        if (denominator < 1e-10) {
            return 0.0;
        }

        double ra = numerator / denominator;

        // Convert to dB and add 2.0 dB offset (A-weighting is 0 dB at 1 kHz)
        // A_dB = 20 * log10(Ra) + 2.0
        // Then convert back to linear gain
        double aDb = 20.0 * std::log10(ra) + 2.0;
        return std::pow(10.0, aDb / 20.0);
    }

    /**
     * @brief Convert FFT bin index to frequency.
     *
     * @param bin Bin index (0 to fftSize/2 - 1)
     * @param fftSize Total FFT size
     * @param sampleRate Sample rate in Hz
     * @return Frequency in Hz
     */
    [[nodiscard]] static double binToFrequency(int bin, int fftSize, double sampleRate) {
        return static_cast<double>(bin) * sampleRate / static_cast<double>(fftSize);
    }

private:
    std::vector<float> weights;
    double cachedSampleRate = 0;
    int cachedFftSize = 0;
};

} // namespace Loudness
