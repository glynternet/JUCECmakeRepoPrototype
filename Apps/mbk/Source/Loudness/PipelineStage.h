#pragma once

namespace Loudness {

/**
 * @brief Interface for a stage in the loudness processing pipeline.
 *
 * Each stage takes a loudness value as input and produces a processed loudness
 * value as output. This abstraction enables:
 * - Uniform interface for all processing stages
 * - Easy iteration for pipeline visualization
 * - Simple extensibility for future stages
 *
 * ## Usage
 *
 * Implement this interface for each processing stage:
 * ```cpp
 * class MyStage : public PipelineStage {
 * public:
 *     float process(float input) override { return transform(input); }
 *     const char* name() const override { return "MyStage"; }
 * };
 * ```
 */
class PipelineStage {
public:
    virtual ~PipelineStage() = default;

    /**
     * @brief Process an input loudness value.
     * @param input Loudness value from previous stage
     * @return Processed loudness value for next stage
     */
    virtual float process(float input) = 0;

    /**
     * @brief Get the display name for this stage.
     * @return Short name for UI display (e.g., "Shaped", "Smoothed")
     */
    [[nodiscard]] virtual const char* name() const = 0;
};

}  // namespace Loudness
