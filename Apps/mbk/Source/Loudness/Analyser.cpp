//
// Created by glynh on 24/11/2022.
//

#include "Analyser.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include "Loudness.h"

namespace Loudness {
Analyser::Analyser(std::function<void(float)> onLoudnessResultCallback,
                   float processingBandIndexLow,
                   float processingBandIndexHigh,
                   float initialSmoothing,
                   float initialDecayExponent)
    : onLoudnessResult(std::move(onLoudnessResultCallback))
    , processingBandLow(processingBandIndexLow)
    , processingBandHigh(processingBandIndexHigh)
    , smoother(initialSmoothing)
    , decayLength(initialDecayExponent) {
    processingThread = std::thread(&Analyser::processingThreadMain, this);
}

Analyser::~Analyser() {
    // Clean shutdown sequence for the processing thread.
    // This pattern ensures no deadlocks or resource leaks.

    {
        // std::scoped_lock: RAII wrapper that acquires the mutex on construction
        // and releases it when the scope ends (even if an exception is thrown).
        // We need the lock because we're modifying shouldStop while the processing
        // thread might be checking it inside the condition_variable wait.
        std::scoped_lock lock(mutex);
        shouldStop = true;
    }
    // Lock released here - processing thread can now acquire it

    // Wake up the processing thread so it can see shouldStop==true and exit.
    // Without this, the thread would sleep forever waiting for data.
    dataReady.notify_one();

    // Wait for the processing thread to finish before destroying the object.
    // joinable() returns false if the thread was never started or already joined.
    // join() blocks until the thread function returns.
    if (processingThread.joinable()) {
        processingThread.join();
    }
}

void Analyser::processingThreadMain() {
    // This function runs on a dedicated thread, separate from the audio thread.
    // It waits for data, processes FFT, and invokes the callback.

    while (true) {
        // std::unique_lock: Like lock_guard but can be manually unlocked.
        // Required for condition_variable::wait() which needs to unlock/relock.
        std::unique_lock<std::mutex> lock(mutex);

        // condition_variable::wait() does three things atomically:
        // 1. Checks the predicate (lambda). If true, continues immediately.
        // 2. If false, releases the lock and puts this thread to sleep.
        // 3. When notify_one() is called, wakes up, re-acquires lock, rechecks predicate.
        // The predicate prevents "spurious wakeups" (OS may wake thread randomly).
        dataReady.wait(lock, [this] { return fftDataPending || shouldStop; });

        if (shouldStop) {
            return;
        }

        // Clear the pending flag while we still hold the lock
        fftDataPending = false;

        // Release lock before heavy processing - allows audio thread to continue
        // filling the FIFO without being blocked by FFT computation.
        lock.unlock();

        // FFT processing (no lock held - thread-safe because fftData was copied)
        window.multiplyWithWindowingTable(fftData, fftSize);
        forwardFFT.performFrequencyOnlyForwardTransform(fftData);
        auto level = calculateLevel();

        if (onLoudnessResult != nullptr) {
            onLoudnessResult(level);
        }
    }
}

// calculateLevel from the FFT data
float Analyser::calculateLevel() {
    auto maxIndex = fftSize / 2;
    // TODO: work out a better "crossover" point as the frequency scale isn't linear
    const auto indexLow =
        static_cast<int>(static_cast<float>(maxIndex) * processingBandLow);
    const auto indexHigh =
        static_cast<int>(static_cast<float>(maxIndex) * processingBandHigh);

    // Apply A-weighting if enabled (modifies fftData in-place)
    if (weightingMode == WeightingMode::AWeighted) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        aWeighting.apply(&fftData[indexLow], indexHigh - indexLow, indexLow);
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    auto level = calculateLoudness(&fftData[indexLow], indexHigh - indexLow);

    // Feed raw level to adapter and sync ranges to ValueShaper
    rangeAdapter.update(level);
    if (rangeAdapter.isEnabled() && !rangeAdapter.isLocked()) {
        // Set observed input range (learned from content)
        valueShaper.setInMin(rangeAdapter.getObservedMin());
        valueShaper.setInMax(rangeAdapter.getObservedMax());
    }
    // Always apply target output range (user-configurable)
    valueShaper.setOutMin(rangeAdapter.getTargetOutMin());
    valueShaper.setOutMax(rangeAdapter.getTargetOutMax());

    level = smoother.add(valueShaper.shape(level));
    level = decayLength.getValue(level);
    return level < 0.0001F ? 0.0F : jlimit(0.0F, 1.0F, level);
}

void Analyser::pushNextSampleIntoFifo(float sample) noexcept {
    // Called from the audio thread at sample rate (e.g., 44,100 Hz).
    // Must be fast and never block for long - audio glitches if we're slow.
    //
    // Note: This function is marked noexcept despite using std::lock_guard.
    // std::mutex::lock() can theoretically throw std::system_error if the mutex
    // is corrupted or a system limit is reached, but in practice this doesn't
    // happen on modern systems with properly initialized mutexes. If it did,
    // std::terminate would be called, which is the correct behavior for an
    // unrecoverable error in audio processing.

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    fifo[fifoIndex++] = sample;

    if (fifoIndex == fftSize) {
        fifoIndex = 0;

        // Critical section: copy data to fftData buffer for processing thread.
        // The lock is held very briefly (just for memcpy ~1KB).
        {
            std::scoped_lock lock(mutex);
            zeromem(fftData, sizeof(fftData));
            memcpy(fftData, fifo, fftSize * sizeof(float));
            fftDataPending = true;
        }

        // notify_one(): Wakes exactly one thread waiting on this condition_variable.
        // If the processing thread is sleeping in wait(), it will wake up immediately.
        // If it's already processing, this is a no-op (the flag is set, so next
        // time it checks the predicate it will see fftDataPending==true).
        dataReady.notify_one();
    }
}

/**
 * @brief Calculate loudness from FFT magnitude data.
 *
 * Dispatches to either perceptual (Stevens' Power Law) or linear calculation
 * based on the current mappingMode setting.
 *
 * - **Perceptual** (default): Uses power domain and Stevens exponent for
 *   true perceptual linearity where 0.5 feels half as loud as 1.0.
 *
 * - **Linear** (legacy): Simple linear amplitude mapping for backward
 *   compatibility. Does NOT achieve perceptual linearity.
 *
 * @param data     Pointer to FFT magnitude values (may be A-weighted)
 * @param dataSize Number of bins to process
 * @return Raw loudness value before range adaptation
 */
float Analyser::calculateLoudness(float* data, int dataSize) const {
    if (mappingMode == MappingMode::Perceptual) {
        // Recommended: Stevens' Power Law for true perceptual linearity
        return LoudnessCalculator::CalculatePerceptual(data, dataSize);
    }

    // Legacy linear mode for backward compatibility
    const auto mindB = -100.0F;
    const auto maxdB = 0.0F;
    // fftSizeInDB is reserved for future normalization features
    float fftSizeInDB = juce::Decibels::gainToDecibels(static_cast<float>(fftSize));
    return LoudnessCalculator::Calculate(data, dataSize, mindB, maxdB, fftSizeInDB);
}
void Analyser::setSampleRate(double rate) {
    sampleRate = rate;
    aWeighting.prepare(fftSize, sampleRate);
}
} // namespace Loudness
