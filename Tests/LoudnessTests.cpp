/**
 * @file LoudnessTests.cpp
 * @brief Unit tests for perceptual loudness calculation using Stevens' Power Law.
 *
 * These tests verify that the perceptual loudness algorithm correctly implements
 * Stevens' Power Law for true perceptual linearity.
 *
 * ## Stevens' Power Law
 *
 * Perceived loudness L relates to physical intensity I by: L = k * I^0.3
 * Since intensity is proportional to amplitude squared: L ∝ amplitude^0.6
 *
 * Key properties verified:
 * - Zero input returns zero output
 * - Maximum reference input returns 1.0
 * - Mid-amplitude produces higher perceptual value than linear would (compression)
 * - 10x intensity increase produces 2x perceived loudness
 * - Output 0.5 corresponds to ~10% of maximum intensity
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <algorithm>
#include <array>
#include <cmath>

namespace {

/**
 * @brief Stevens' Power Law exponent for intensity (power domain).
 *
 * The canonical value from psychoacoustic research. When applied to
 * normalized power (amplitude²), produces perceptually linear output.
 */
constexpr float kStevensExponent = 0.3F;

/**
 * @brief Reference maximum power for normalization.
 *
 * Derived from empirical maximum amplitude of 25.0:
 * kReferencePower = 25² = 625
 */
constexpr float kReferencePower = 625.0F;

/**
 * @brief Calculate perceptual loudness using Stevens' Power Law.
 *
 * This is the test implementation that mirrors the production code in Loudness.h.
 *
 * Algorithm:
 * 1. Square each magnitude to get power (intensity ∝ amplitude²)
 * 2. Average power across all bins
 * 3. Normalize to [0, 1] using kReferencePower
 * 4. Apply Stevens' Power Law: perceived = power^0.3
 *
 * @param data     Pointer to FFT magnitude values
 * @param dataSize Number of FFT bins to process
 * @return Perceptual loudness in range [0, 1]
 */
float calculatePerceptual(const float* data, int dataSize) {
    if (dataSize <= 0 || data == nullptr) {
        return 0.0F;
    }

    // Sum power (amplitude²) across bins
    float sumPower = 0.0F;
    for (int i = 0; i < dataSize; ++i) {
        sumPower += data[i] * data[i];
    }

    // Average and normalize
    float avgPower = sumPower / static_cast<float>(dataSize);
    float normalizedPower = std::min(avgPower / kReferencePower, 1.0F);

    // Apply Stevens' Power Law
    return std::pow(normalizedPower, kStevensExponent);
}

/**
 * @brief Legacy linear loudness calculation for comparison.
 *
 * Maps each magnitude linearly from [0, 25] to [0, 1] and averages.
 * Does NOT achieve perceptual linearity.
 */
float calculateLinear(const float* data, int dataSize) {
    if (dataSize <= 0 || data == nullptr) {
        return 0.0F;
    }

    float sum = 0.0F;
    for (int i = 0; i < dataSize; ++i) {
        // Linear mapping: [0, 25] -> [0, 1]
        float level = std::clamp(data[i] / 25.0F, 0.0F, 1.0F);
        sum += level;
    }
    return sum / static_cast<float>(dataSize);
}

} // anonymous namespace

// =============================================================================
// Basic Functionality Tests
// =============================================================================

TEST_CASE("Perceptual loudness - zero input", "[loudness][perceptual]") {
    SECTION("Single zero returns zero") {
        std::array<float, 1> data = {0.0F};
        REQUIRE(calculatePerceptual(data.data(), 1) == 0.0F);
    }

    SECTION("Multiple zeros return zero") {
        std::array<float, 4> data = {0.0F, 0.0F, 0.0F, 0.0F};
        REQUIRE(calculatePerceptual(data.data(), 4) == 0.0F);
    }

    SECTION("Empty input returns zero") {
        REQUIRE(calculatePerceptual(nullptr, 0) == 0.0F);
    }

    SECTION("Null pointer returns zero") {
        REQUIRE(calculatePerceptual(nullptr, 10) == 0.0F);
    }
}

TEST_CASE("Perceptual loudness - maximum input", "[loudness][perceptual]") {
    SECTION("Single maximum amplitude (25) returns 1.0") {
        std::array<float, 1> data = {25.0F};
        REQUIRE_THAT(calculatePerceptual(data.data(), 1),
                     Catch::Matchers::WithinAbs(1.0F, 0.001F));
    }

    SECTION("Multiple maximum amplitudes return 1.0") {
        std::array<float, 4> data = {25.0F, 25.0F, 25.0F, 25.0F};
        REQUIRE_THAT(calculatePerceptual(data.data(), 4),
                     Catch::Matchers::WithinAbs(1.0F, 0.001F));
    }

    SECTION("Above reference is clamped to 1.0") {
        std::array<float, 1> data = {50.0F}; // Power = 2500, above 625 reference
        REQUIRE(calculatePerceptual(data.data(), 1) == 1.0F);
    }
}

// =============================================================================
// Stevens' Power Law Properties
// =============================================================================

TEST_CASE("Perceptual loudness - Stevens compression", "[loudness][perceptual][stevens]") {
    SECTION("Mid-amplitude produces higher perceptual than linear") {
        // At amplitude 12.5 (half of max 25):
        // - Linear: 12.5/25 = 0.5
        // - Perceptual: (12.5²/625)^0.3 = (156.25/625)^0.3 = 0.25^0.3 ≈ 0.66
        std::array<float, 1> data = {12.5F};

        float linear = calculateLinear(data.data(), 1);
        float perceptual = calculatePerceptual(data.data(), 1);

        // Linear gives 0.5
        REQUIRE_THAT(linear, Catch::Matchers::WithinAbs(0.5F, 0.01F));

        // Perceptual is higher (compressed response)
        REQUIRE(perceptual > linear);
        REQUIRE_THAT(perceptual, Catch::Matchers::WithinAbs(0.66F, 0.03F));
    }

    SECTION("Quarter amplitude shows even stronger compression") {
        // At amplitude 6.25 (quarter of max 25):
        // - Linear: 6.25/25 = 0.25
        // - Perceptual: (6.25²/625)^0.3 = (39.0625/625)^0.3 = 0.0625^0.3 ≈ 0.46
        std::array<float, 1> data = {6.25F};

        float linear = calculateLinear(data.data(), 1);
        float perceptual = calculatePerceptual(data.data(), 1);

        REQUIRE_THAT(linear, Catch::Matchers::WithinAbs(0.25F, 0.01F));
        REQUIRE(perceptual > linear);
        REQUIRE_THAT(perceptual, Catch::Matchers::WithinAbs(0.46F, 0.03F));
    }
}

TEST_CASE("Perceptual loudness - half perceived at ~10% intensity",
          "[loudness][perceptual][stevens]") {
    // Key property of Stevens' Power Law:
    // perceived = 0.5 when normalizedPower^0.3 = 0.5
    // normalizedPower = 0.5^(1/0.3) = 0.5^3.333 ≈ 0.099
    // So ~10% of max power gives 50% perceived loudness

    float targetNormalizedPower = std::pow(0.5F, 1.0F / kStevensExponent);
    float targetPower = targetNormalizedPower * kReferencePower;
    float amplitude = std::sqrt(targetPower);

    std::array<float, 1> data = {amplitude};
    float result = calculatePerceptual(data.data(), 1);

    REQUIRE_THAT(result, Catch::Matchers::WithinAbs(0.5F, 0.02F));

    // Verify the amplitude is roughly 7.87 (sqrt of ~62)
    REQUIRE_THAT(amplitude, Catch::Matchers::WithinAbs(7.87F, 0.1F));
}

TEST_CASE("Perceptual loudness - 10x intensity doubles perception",
          "[loudness][perceptual][stevens]") {
    // Key property: 10x intensity increase = 2x perceived loudness
    // This comes from: (10*I)^0.3 / I^0.3 = 10^0.3 ≈ 2.0

    float baseAmplitude = 5.0F;
    float basePower = baseAmplitude * baseAmplitude;

    // 10x intensity means 10x power, which means sqrt(10) times amplitude
    float tenXAmplitude = std::sqrt(basePower * 10.0F);

    std::array<float, 1> baseData = {baseAmplitude};
    std::array<float, 1> tenXData = {tenXAmplitude};

    float basePerceived = calculatePerceptual(baseData.data(), 1);
    float tenXPerceived = calculatePerceptual(tenXData.data(), 1);

    float ratio = tenXPerceived / basePerceived;

    // 10^0.3 = 1.995... ≈ 2.0
    REQUIRE_THAT(ratio, Catch::Matchers::WithinAbs(2.0F, 0.1F));
}

TEST_CASE("Perceptual loudness - doubling perceived requires 10x intensity",
          "[loudness][perceptual][stevens]") {
    // Inverse of above: to double perceived loudness, need 10x intensity
    // If perceived_2 = 2 * perceived_1, then I_2/I_1 = 2^(1/0.3) = 2^3.333 ≈ 10.08

    float baseAmplitude = 5.0F;
    float basePower = baseAmplitude * baseAmplitude;

    // Calculate amplitude needed for 2x perceived loudness
    float intensityMultiplier = std::pow(2.0F, 1.0F / kStevensExponent);
    float doubledAmplitude = std::sqrt(basePower * intensityMultiplier);

    std::array<float, 1> baseData = {baseAmplitude};
    std::array<float, 1> doubledData = {doubledAmplitude};

    float basePerceived = calculatePerceptual(baseData.data(), 1);
    float doubledPerceived = calculatePerceptual(doubledData.data(), 1);

    float ratio = doubledPerceived / basePerceived;

    REQUIRE_THAT(ratio, Catch::Matchers::WithinAbs(2.0F, 0.05F));

    // Verify the intensity multiplier is ~10
    REQUIRE_THAT(intensityMultiplier, Catch::Matchers::WithinAbs(10.08F, 0.1F));
}

// =============================================================================
// Power Domain Averaging Tests
// =============================================================================

TEST_CASE("Perceptual loudness - power domain averaging", "[loudness][perceptual]") {
    SECTION("Two sources combine correctly in power domain") {
        // Two sources: amplitude 10 and 15
        // Power averaging: (100 + 225) / 2 = 162.5
        // Normalized: 162.5 / 625 = 0.26
        // Perceived: 0.26^0.3 ≈ 0.68

        std::array<float, 2> data = {10.0F, 15.0F};
        float result = calculatePerceptual(data.data(), 2);

        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(0.68F, 0.02F));
    }

    SECTION("Uniform distribution across bins") {
        // All bins at amplitude 10: power = 100 each
        // avgPower = 100, normalized = 100/625 = 0.16
        // perceived = 0.16^0.3 ≈ 0.58
        std::array<float, 4> data = {10.0F, 10.0F, 10.0F, 10.0F};
        float result = calculatePerceptual(data.data(), 4);

        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(0.58F, 0.02F));
    }

    SECTION("Single loud bin among quiet ones") {
        // One loud bin (20) among quiet ones (5)
        // Powers: 400, 25, 25, 25 -> avg = 118.75
        // Normalized: 118.75/625 = 0.19
        // Perceived: 0.19^0.3 ≈ 0.61

        std::array<float, 4> data = {20.0F, 5.0F, 5.0F, 5.0F};
        float result = calculatePerceptual(data.data(), 4);

        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(0.61F, 0.02F));
    }
}

// =============================================================================
// Linear Mode Tests (for comparison)
// =============================================================================

TEST_CASE("Linear loudness - basic functionality", "[loudness][linear]") {
    SECTION("Zero input returns zero") {
        std::array<float, 4> data = {0.0F, 0.0F, 0.0F, 0.0F};
        REQUIRE(calculateLinear(data.data(), 4) == 0.0F);
    }

    SECTION("Maximum input (25) returns 1.0") {
        std::array<float, 4> data = {25.0F, 25.0F, 25.0F, 25.0F};
        REQUIRE_THAT(calculateLinear(data.data(), 4),
                     Catch::Matchers::WithinAbs(1.0F, 0.001F));
    }

    SECTION("Mid-range input (12.5) returns 0.5") {
        std::array<float, 4> data = {12.5F, 12.5F, 12.5F, 12.5F};
        REQUIRE_THAT(calculateLinear(data.data(), 4),
                     Catch::Matchers::WithinAbs(0.5F, 0.001F));
    }

    SECTION("Mixed values average correctly") {
        std::array<float, 2> data = {0.0F, 25.0F};
        REQUIRE_THAT(calculateLinear(data.data(), 2),
                     Catch::Matchers::WithinAbs(0.5F, 0.001F));
    }
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_CASE("Perceptual loudness - edge cases", "[loudness][perceptual][edge]") {
    SECTION("Single sample works correctly") {
        std::array<float, 1> data = {10.0F};
        float result = calculatePerceptual(data.data(), 1);

        // power = 100, normalized = 100/625 = 0.16, perceived = 0.16^0.3 ≈ 0.58
        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(0.58F, 0.02F));
    }

    SECTION("Very small values produce very small output") {
        std::array<float, 1> data = {0.1F};
        float result = calculatePerceptual(data.data(), 1);

        // power = 0.01, normalized = 0.01/625 = 0.000016
        // perceived = 0.000016^0.3 ≈ 0.037
        REQUIRE(result < 0.05F);
        REQUIRE(result > 0.0F);
    }

    SECTION("Large number of bins handles correctly") {
        std::array<float, 128> data;
        data.fill(10.0F);
        float result = calculatePerceptual(data.data(), 128);

        // Same as single bin at 10: 0.58
        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(0.58F, 0.02F));
    }
}
