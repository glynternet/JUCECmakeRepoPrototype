/**
 * @file LoudnessIndexTest.cpp
 * @brief Unit tests for LoudnessIndex class.
 *
 * Tests verify:
 * - Basic construction and initial values
 * - EWMA calculation at different time scales
 * - Sample rate configuration affects alpha values
 * - Range tracking
 * - Output scale (0-10)
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../Apps/mbk/Source/Loudness/LoudnessIndex.h"

using Loudness::LoudnessIndex;

// =============================================================================
// Construction and Initial Values
// =============================================================================

TEST_CASE("LoudnessIndex - construction", "[loudness-index][construction]") {
    SECTION("Initial index values are 5.0 (mid-scale)") {
        LoudnessIndex index;

        // Initial EMA is 0.5 internal, scaled to 5.0
        REQUIRE_THAT(index.getIndex10s(), Catch::Matchers::WithinAbs(5.0f, 0.01f));
        REQUIRE_THAT(index.getIndex1m(), Catch::Matchers::WithinAbs(5.0f, 0.01f));
        REQUIRE_THAT(index.getIndex5m(), Catch::Matchers::WithinAbs(5.0f, 0.01f));
    }

    SECTION("Initial range is close to minimum separation") {
        LoudnessIndex index;

        // AdaptiveRange starts at 0.0-1.0 with 0.05 min separation
        // getRange() returns internal range * 10
        REQUIRE(index.getRange() >= 0.0f);
        REQUIRE(index.getRange() <= 10.0f);
    }

    SECTION("Custom FFT size is accepted") {
        LoudnessIndex index(512);  // Different FFT size

        // Should still have valid initial values
        REQUIRE_THAT(index.getIndex10s(), Catch::Matchers::WithinAbs(5.0f, 0.01f));
    }
}

// =============================================================================
// EWMA Update Behavior
// =============================================================================

TEST_CASE("LoudnessIndex - EWMA updates", "[loudness-index][ewma]") {
    SECTION("High values increase all indices") {
        LoudnessIndex index;
        float initial10s = index.getIndex10s();

        // Feed high values
        for (int i = 0; i < 100; ++i) {
            index.update(1.0f);
        }

        REQUIRE(index.getIndex10s() > initial10s);
        REQUIRE(index.getIndex1m() > 5.0f);
        REQUIRE(index.getIndex5m() > 5.0f);
    }

    SECTION("Low values decrease all indices") {
        LoudnessIndex index;
        float initial10s = index.getIndex10s();

        // Feed low values
        for (int i = 0; i < 100; ++i) {
            index.update(0.0f);
        }

        REQUIRE(index.getIndex10s() < initial10s);
        REQUIRE(index.getIndex1m() < 5.0f);
        REQUIRE(index.getIndex5m() < 5.0f);
    }

    SECTION("10s index responds faster than 1m and 5m") {
        LoudnessIndex index;

        // Feed high values for a short time
        for (int i = 0; i < 50; ++i) {
            index.update(1.0f);
        }

        float delta10s = index.getIndex10s() - 5.0f;
        float delta1m = index.getIndex1m() - 5.0f;
        float delta5m = index.getIndex5m() - 5.0f;

        // 10s should have moved more than 1m, which moved more than 5m
        REQUIRE(delta10s > delta1m);
        REQUIRE(delta1m > delta5m);
    }

    SECTION("Values stabilize at constant input") {
        LoudnessIndex index;

        // Feed constant value for a long time
        // At 44.1kHz with 256 FFT, ~172 updates/sec, so 1 minute needs ~10,320 samples
        for (int i = 0; i < 20000; ++i) {
            index.update(0.7f);
        }

        // All indices should approach 7.0 (0.7 * 10)
        REQUIRE_THAT(index.getIndex10s(), Catch::Matchers::WithinAbs(7.0f, 0.5f));
        REQUIRE_THAT(index.getIndex1m(), Catch::Matchers::WithinAbs(7.0f, 1.0f));
        // 5m needs more samples to stabilize
    }
}

// =============================================================================
// Sample Rate Configuration
// =============================================================================

TEST_CASE("LoudnessIndex - sample rate", "[loudness-index][sample-rate]") {
    SECTION("setSampleRate changes response rate") {
        LoudnessIndex indexLow(256);
        LoudnessIndex indexHigh(256);

        indexLow.setSampleRate(22050.0);   // ~86 updates/sec
        indexHigh.setSampleRate(96000.0);  // ~375 updates/sec

        // Feed same number of updates
        for (int i = 0; i < 100; ++i) {
            indexLow.update(1.0f);
            indexHigh.update(1.0f);
        }

        // Lower sample rate = fewer expected updates = larger alpha = faster response
        // At 22050 Hz: 100 updates represent more "time" than at 96000 Hz
        REQUIRE(indexLow.getIndex10s() > indexHigh.getIndex10s());
    }

    SECTION("Different FFT sizes affect response") {
        LoudnessIndex small(128);   // More updates per second
        LoudnessIndex large(512);   // Fewer updates per second

        // Same sample rate
        small.setSampleRate(48000.0);  // 375 updates/sec
        large.setSampleRate(48000.0);  // 93.75 updates/sec

        for (int i = 0; i < 100; ++i) {
            small.update(1.0f);
            large.update(1.0f);
        }

        // Larger FFT = fewer expected updates = larger alpha = faster response per update
        REQUIRE(large.getIndex10s() > small.getIndex10s());
    }
}

// =============================================================================
// Range Tracking
// =============================================================================

TEST_CASE("LoudnessIndex - range tracking", "[loudness-index][range]") {
    SECTION("Range reflects input variance") {
        LoudnessIndex index;

        // Feed alternating high and low values
        for (int i = 0; i < 200; ++i) {
            index.update(i % 2 == 0 ? 0.1f : 0.9f);
        }

        // Range should be substantial (reflects 0.1-0.9 input variance)
        REQUIRE(index.getRange() > 5.0f);
    }

    SECTION("Range contracts with constant input") {
        LoudnessIndex index;

        // First establish some variance
        for (int i = 0; i < 100; ++i) {
            index.update(i % 2 == 0 ? 0.0f : 1.0f);
        }
        float expandedRange = index.getRange();

        // Then contract with constant values
        for (int i = 0; i < 500; ++i) {
            index.update(0.5f);
        }

        REQUIRE(index.getRange() < expandedRange);
    }

    SECTION("Range is scaled to 0-10") {
        LoudnessIndex index;

        // With full 0.0 to 1.0 input range, output should be near 10
        for (int i = 0; i < 200; ++i) {
            index.update(i % 2 == 0 ? 0.0f : 1.0f);
        }

        REQUIRE(index.getRange() <= 10.0f);
        REQUIRE(index.getRange() >= 0.0f);
    }
}

// =============================================================================
// Output Scale
// =============================================================================

TEST_CASE("LoudnessIndex - output scale", "[loudness-index][scale]") {
    SECTION("All outputs are in 0-10 range") {
        LoudnessIndex index;

        // Feed various values
        for (int i = 0; i < 100; ++i) {
            float value = static_cast<float>(i) / 100.0f;
            index.update(value);

            REQUIRE(index.getIndex10s() >= 0.0f);
            REQUIRE(index.getIndex10s() <= 10.0f);
            REQUIRE(index.getIndex1m() >= 0.0f);
            REQUIRE(index.getIndex1m() <= 10.0f);
            REQUIRE(index.getIndex5m() >= 0.0f);
            REQUIRE(index.getIndex5m() <= 10.0f);
            REQUIRE(index.getRange() >= 0.0f);
            REQUIRE(index.getRange() <= 10.0f);
        }
    }

    SECTION("Full input range produces full output range over time") {
        LoudnessIndex index;

        // Feed max value for extended period
        for (int i = 0; i < 50000; ++i) {
            index.update(1.0f);
        }

        // 10s should be very close to 10.0
        REQUIRE_THAT(index.getIndex10s(), Catch::Matchers::WithinAbs(10.0f, 0.5f));
    }
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_CASE("LoudnessIndex - edge cases", "[loudness-index][edge]") {
    SECTION("Handles zero input") {
        LoudnessIndex index;

        for (int i = 0; i < 100; ++i) {
            index.update(0.0f);
        }

        REQUIRE(std::isfinite(index.getIndex10s()));
        REQUIRE(std::isfinite(index.getIndex1m()));
        REQUIRE(std::isfinite(index.getIndex5m()));
        REQUIRE(std::isfinite(index.getRange()));
    }

    SECTION("Handles input at boundary") {
        LoudnessIndex index;

        index.update(0.0f);
        index.update(1.0f);

        REQUIRE(std::isfinite(index.getIndex10s()));
        REQUIRE(std::isfinite(index.getRange()));
    }

    SECTION("Stable under many updates") {
        LoudnessIndex index;

        for (int i = 0; i < 100000; ++i) {
            float value = static_cast<float>(i % 100) / 100.0f;
            index.update(value);
        }

        REQUIRE(std::isfinite(index.getIndex10s()));
        REQUIRE(std::isfinite(index.getIndex1m()));
        REQUIRE(std::isfinite(index.getIndex5m()));
        REQUIRE(std::isfinite(index.getRange()));
    }
}
