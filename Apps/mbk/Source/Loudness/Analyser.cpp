//
// Created by glynh on 24/11/2022.
//

#include "Analyser.h"
#include <cstring>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Loudness.h"

namespace Loudness {
Analyser::Analyser(std::function<void(float)> onLoudnessResultCallback,
                   float processingBandIndexLow,
                   float processingBandIndexHigh,
                   float initialSmoothing,
                   float initialDecayExponent,
                   juce::Range<float> initialRangeIn,
                   juce::Range<float> initialOutput,
                   float initialAdaptationRate,
                   WeightingMode initialWeightingMode,
                   MappingMode initialMappingMode)
    : onLoudnessResult(std::move(onLoudnessResultCallback))
    , processingBandLow(processingBandIndexLow)
    , processingBandHigh(processingBandIndexHigh)
    , inputRange(initialRangeIn, initialAdaptationRate)
    , targetOutMin(initialOutput.getStart())
    , targetOutMax(initialOutput.getEnd())
    , smoother(initialSmoothing)
    , decayLength(initialDecayExponent)
    , mappingMode(initialMappingMode)
    , weightingMode(initialWeightingMode) {
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
    // It implements a "wait then drain" pattern:
    // 1. Block until frames are queued (efficient, zero CPU when idle)
    // 2. Process ALL queued frames before going back to sleep
    //
    // The "drain all" approach is important: if we only processed one frame per
    // wakeup, we'd fall behind during bursts from large audio buffers.

    while (true) {
        // Block until work is available or shutdown requested.
        //
        // std::unique_lock: Required for condition_variable (unlike scoped_lock,
        // it can be unlocked/relocked by wait()).
        //
        // condition_variable::wait() atomically:
        // 1. Checks predicate - if true, returns immediately (no sleep)
        // 2. If false, releases mutex and puts thread to sleep (zero CPU)
        // 3. When notify_one() called, wakes up, re-acquires mutex, rechecks
        //
        // The predicate lambda prevents "spurious wakeups" - the OS may wake
        // the thread randomly, but we only proceed if there's actual work.
        {
            std::unique_lock<std::mutex> lock(mutex);
            dataReady.wait(lock, [this] {
                return frameQueueFifo.getNumReady() > 0 || shouldStop;
            });
        }
        // Mutex released here - we don't hold it during FFT processing

        if (shouldStop) {
            return;
        }

        // Drain all queued frames. The lock-free AbstractFifo allows the audio
        // thread to continue queueing frames while we process, so new frames
        // may arrive during this loop. We process everything available before
        // going back to sleep.
        while (frameQueueFifo.getNumReady() > 0) {
            // AbstractFifo::read() returns a ScopedRead with indices into the
            // circular buffer. blockSize1 is the contiguous block before wrap,
            // blockSize2 (if any) is after wrap. For single-frame reads,
            // blockSize2 is always 0.
            const auto scope = frameQueueFifo.read(1);
            if (scope.blockSize1 > 0) {
                // Copy frame data to local processing buffer. This is the only
                // "copy" in the data path - the queue itself is zero-copy.
                juce::zeromem(fftData, sizeof(fftData));
                std::memcpy(
                    fftData,
                    frameQueue.at(static_cast<size_t>(scope.startIndex1)).data(),
                    fftSize * sizeof(float));
            }
            // ScopedRead destructor automatically advances the read pointer

            // FFT processing (computationally expensive, ~0.5-1ms)
            window.multiplyWithWindowingTable(fftData, fftSize);
            forwardFFT.performFrequencyOnlyForwardTransform(fftData);
            auto level = calculateLevel();

            // Update long-term loudness statistics
            loudnessIndex.update(level);

            if (onLoudnessResult != nullptr) {
                onLoudnessResult(level);
            }

            if (onIndexUpdate != nullptr) {
                onIndexUpdate(loudnessIndex.getIndex10s(),
                              loudnessIndex.getIndex1m(),
                              loudnessIndex.getIndex5m(),
                              loudnessIndex.getRange());
            }

            // Check shutdown between frames for responsive termination.
            // Without this, destructor would block until all queued frames
            // are processed, which could take 10+ ms with a full queue.
            if (shouldStop) {
                return;
            }
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
    auto rawLevel = calculateLoudness(&fftData[indexLow], indexHigh - indexLow);

    // Feed raw level to adaptive range (only learns if enabled && !locked)
    inputRange.update(rawLevel);
    // Always apply current input range to ValueShaper (whether from auto or manual)
    valueShaper.setInputRange(inputRange.getBounds());
    // Always apply target output range (user-configurable)
    valueShaper.setOutputRange(getTargetOutRange());

    // Process through pipeline stages, capturing intermediate values
    auto shapedLevel = valueShaper.process(rawLevel);
    auto smoothedLevel = smoother.process(shapedLevel);
    auto decayedLevel = decayLength.process(smoothedLevel);
    auto finalLevel = clamp.process(decayedLevel);

    // Invoke pipeline callback if registered
    if (onPipelineUpdate != nullptr) {
        onPipelineUpdate({rawLevel, shapedLevel, smoothedLevel, decayedLevel, finalLevel});
    }

    return finalLevel;
}

void Analyser::pushNextSampleIntoFifo(float sample) noexcept {
    // Called from the audio thread at sample rate (e.g., 44,100 Hz).
    // CRITICAL: Must be fast and never block - any delay causes audio glitches.
    //
    // This function is marked noexcept because exceptions in the audio thread
    // would be unrecoverable. If something goes catastrophically wrong,
    // std::terminate is the correct behavior.

    // Accumulate samples in local buffer (not shared with other threads)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    fifo[fifoIndex++] = sample;

    if (fifoIndex == fftSize) {
        fifoIndex = 0;

        // Queue the completed frame using lock-free AbstractFifo.
        //
        // AbstractFifo::write() returns a ScopedWrite containing indices into
        // the circular buffer. The write is "prepared" but not "committed"
        // until the ScopedWrite destructor runs (RAII pattern).
        //
        // blockSize1: Number of slots available before buffer wraparound
        // blockSize2: Additional slots after wraparound (for multi-item writes)
        // For single-frame writes, we only use blockSize1.
        const auto scope = frameQueueFifo.write(1);
        if (scope.blockSize1 > 0) {
            // Copy frame to queue slot. This is the producer side of the
            // lock-free SPSC (single-producer single-consumer) pattern.
            std::memcpy(frameQueue.at(static_cast<size_t>(scope.startIndex1)).data(),
                        fifo,
                        fftSize * sizeof(float));
        } else {
            // Queue completely full - frame must be dropped.
            // This should be rare with proper queue sizing (32 frames).
            // If it happens frequently, CPU can't keep up with audio rate.
            return;
        }
        // ScopedWrite destructor commits the write (advances write pointer)

        // =====================================================================
        // Overload Detection
        // =====================================================================
        // Track whether the queue stays highly filled across consecutive writes.
        // This distinguishes normal bursts (large audio buffers) from genuine
        // CPU overload (processing thread can't keep up with sample rate).
        //
        // All atomic operations use memory_order_relaxed because:
        // - No synchronization with other threads is needed
        // - We only need eventual consistency for the overload flag
        // - Relaxed is fastest (no memory barriers on most architectures)
        const int numQueued = frameQueueFifo.getNumReady();
        if (numQueued >= highFillThreshold) {
            // fetch_add returns the OLD value, so +1 gives current count
            const int count =
                consecutiveHighFill.fetch_add(1, std::memory_order_relaxed) + 1;
            if (count >= overloadTriggerCount
                && !persistentOverload.load(std::memory_order_relaxed)) {
                persistentOverload.store(true, std::memory_order_relaxed);
                // Note: Cannot invoke onOverloadStateChanged here - callbacks
                // may allocate memory or do other non-realtime-safe operations.
                // The flag can be polled from a timer or UI thread instead.
            }
        } else {
            // Queue drained below threshold - system is keeping up
            consecutiveHighFill.store(0, std::memory_order_relaxed);
            if (persistentOverload.load(std::memory_order_relaxed)) {
                persistentOverload.store(false, std::memory_order_relaxed);
            }
        }

        // Wake the processing thread. notify_one() is very fast (~nanoseconds)
        // and safe to call from the audio thread. If the processing thread is:
        // - Sleeping in wait(): It wakes immediately and starts processing
        // - Already processing: No effect (it will check queue after current frame)
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
    loudnessIndex.setSampleRate(sampleRate);
}
} // namespace Loudness
