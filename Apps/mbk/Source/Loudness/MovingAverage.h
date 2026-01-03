namespace Loudness {

/**
 * @brief Simple moving average filter for smoothing loudness values.
 *
 * Maintains a circular buffer of recent values and returns their average.
 * Used in the loudness processing chain to reduce noise and jitter in
 * the output signal.
 *
 * ## Usage in Processing Chain
 *
 * The MovingAverage sits after value shaping and before decay:
 * ```
 * Shaped Value → MovingAverage → Smoother Value → TailOff
 * ```
 *
 * ## Trade-offs
 *
 * - **Larger window (up to 7)**: Smoother output, but slower response to changes
 * - **Smaller window (1-2)**: Faster response, but more jittery output
 *
 * Default window of 2 provides minimal smoothing while reducing single-sample spikes.
 *
 * @note Maximum window size is 32 samples (hardcoded limit).
 */
class MovingAverage {
private:
    const static unsigned int maxWindow = 32;
    float _window[maxWindow] = {};
    int _index = 0;
    int _period;
    float sum = 0.0F;

public:
    /**
     * @param period Number of samples to average (1-32, clamped if out of range)
     */
    MovingAverage(unsigned int period) {
        if (period < 1) {
            _period = 1;
            // TODO: throw something?
            return;
        }

        if (period > maxWindow) {
            _period = maxWindow;
            // TODO: throw something?
            return;
        }

        _period = period;
    }
    ~MovingAverage() {}

    /** @brief Add a new value to the buffer (circular, overwrites oldest) */
    void add(const float value) {
        if (++_index >= maxWindow) {
            _index -= maxWindow;
        }
        _window[_index] = value;
    }

    /** @brief Calculate average of the last `period` values */
    float getAverage() {
        sum = 0.0F;
        int index = _index;

        for (int i = _period - 1; i >= 0; i--) {
            if (--index < 0)
                index += maxWindow; // Wrap around the full buffer, not just _period
            sum += _window[index];
        }

        return sum / _period;
    }

    int getPeriod() { return _period; }

    /** @brief Change the averaging window size (1-32, clamped if out of range) */
    void setPeriod(const unsigned int period) {
        if (period < 1) {
            _period = 1;
            // TODO: throw something?
            return;
        }

        if (period > maxWindow) {
            _period = maxWindow;
            // TODO: throw something?
            return;
        }

        _period = period;
    }
};
} // namespace Loudness
