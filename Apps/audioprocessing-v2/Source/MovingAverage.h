
class MovingAverage {
   private:
    const static unsigned int maxWindow = 32;
    float _window[maxWindow] = {};
    int _index = 0;
    int _period;
    float sum = 0.f;

   public:
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

    void add(const float value) {
        if (++_index >= maxWindow) {
            _index -= maxWindow;
        }
        _window[_index] = value;
    }

    float getAverage() {
        sum = 0.0f;
        int index = _index;

        for (int i = _period - 1; i >= 0; i--) {
            if (--index < 0) index += _period;
            sum += _window[index];
        }

        return sum / _period;
    }

    int getPeriod() { return _period; }
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