/**
 * @file AdaptiveRangeTest.cpp
 * @brief Unit tests for AdaptiveRange class.
 *
 * Tests verify:
 * - Basic construction and initial values
 * - Range expansion when values exceed bounds
 * - Range contraction when values are within bounds
 * - Asymmetric adaptation rates
 * - Minimum separation enforcement
 * - Enable/disable and lock functionality
 * - Direct bounds manipulation
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../Apps/mbk/Source/Loudness/AdaptiveRange.h"

using Loudness::AdaptiveRange;

// =============================================================================
// Construction and Initial Values
// =============================================================================

TEST_CASE("AdaptiveRange - construction", "[adaptive-range][construction]") {
    SECTION("Initial values match constructor arguments") {
        AdaptiveRange range({0.2f, 0.8f}, 0.01f);

        REQUIRE_THAT(range.getMin(), Catch::Matchers::WithinAbs(0.2f, 0.001f));
        REQUIRE_THAT(range.getMax(), Catch::Matchers::WithinAbs(0.8f, 0.001f));
    }

    SECTION("Width and center calculated correctly") {
        AdaptiveRange range({0.2f, 0.8f}, 0.01f);

        REQUIRE_THAT(range.getWidth(), Catch::Matchers::WithinAbs(0.6f, 0.001f));
        REQUIRE_THAT(range.getCenter(), Catch::Matchers::WithinAbs(0.5f, 0.001f));
    }

    SECTION("Default enabled state is true") {
        AdaptiveRange range({0.0f, 1.0f}, 0.01f);

        REQUIRE(range.isEnabled() == true);
    }

    SECTION("Default locked state is false") {
        AdaptiveRange range({0.0f, 1.0f}, 0.01f);

        REQUIRE(range.isLocked() == false);
    }

    SECTION("Adaptation rate matches constructor argument") {
        AdaptiveRange range({0.0f, 1.0f}, 0.05f);

        REQUIRE_THAT(range.getAdaptationRate(),
                     Catch::Matchers::WithinAbs(0.05f, 0.0001f));
    }
}

// =============================================================================
// Range Expansion (Fast)
// =============================================================================

TEST_CASE("AdaptiveRange - expansion", "[adaptive-range][expansion]") {
    SECTION("Max expands when value exceeds it") {
        AdaptiveRange range({0.2f, 0.5f}, 0.1f);
        float originalMax = range.getMax();

        // Feed value above max
        range.update(0.8f);

        REQUIRE(range.getMax() > originalMax);
    }

    SECTION("Min expands when value is below it") {
        AdaptiveRange range({0.3f, 0.7f}, 0.1f);
        float originalMin = range.getMin();

        // Feed value below min
        range.update(0.1f);

        REQUIRE(range.getMin() < originalMin);
    }

    SECTION("Expansion is relatively fast") {
        AdaptiveRange range({0.4f, 0.6f}, 0.1f);

        // Feed consistently high values
        for (int i = 0; i < 20; ++i) {
            range.update(0.9f);
        }

        // Max should approach 0.9 fairly quickly
        REQUIRE(range.getMax() > 0.75f);
    }
}

// =============================================================================
// Range Contraction (Slow)
// =============================================================================

TEST_CASE("AdaptiveRange - contraction", "[adaptive-range][contraction]") {
    SECTION("Range contracts slowly when values stay within bounds") {
        AdaptiveRange range({0.1f, 0.9f}, 0.1f);
        float originalWidth = range.getWidth();

        // Feed centered values repeatedly
        for (int i = 0; i < 50; ++i) {
            range.update(0.5f);
        }

        // Range should contract (min increases, max decreases)
        REQUIRE(range.getWidth() < originalWidth);
    }

    SECTION("Contraction is slower than expansion") {
        // Test that expansion happens faster than contraction
        AdaptiveRange expandRange({0.4f, 0.6f}, 0.1f);
        AdaptiveRange contractRange({0.2f, 0.8f}, 0.1f);

        // Expand: feed values above max
        for (int i = 0; i < 10; ++i) {
            expandRange.update(0.9f);
        }
        float expandDelta = expandRange.getMax() - 0.6f;

        // Contract: feed centered values
        for (int i = 0; i < 10; ++i) {
            contractRange.update(0.5f);
        }
        float contractDelta = 0.8f - contractRange.getMax();

        // Expansion should be more significant than contraction
        REQUIRE(expandDelta > contractDelta);
    }
}

// =============================================================================
// Minimum Separation
// =============================================================================

TEST_CASE("AdaptiveRange - minimum separation", "[adaptive-range][separation]") {
    SECTION("Enforces minimum separation with default value") {
        AdaptiveRange range({0.5f, 0.5f}, 0.1f);  // Degenerate initial range

        range.update(0.5f);

        // Should enforce default minimum separation of 0.3
        REQUIRE(range.getWidth() >= 0.3f - 0.0001f);
    }

    SECTION("Enforces custom minimum separation") {
        float customSeparation = 0.2f;
        AdaptiveRange range({0.5f, 0.55f}, 0.1f, customSeparation);

        range.update(0.5f);

        REQUIRE(range.getWidth() >= customSeparation);
    }

    SECTION("Range cannot contract below minimum separation") {
        AdaptiveRange range({0.3f, 0.7f}, 0.1f, 0.1f);

        // Feed many centered values to try to contract
        for (int i = 0; i < 500; ++i) {
            range.update(0.5f);
        }

        // Range should not go below minimum separation
        REQUIRE(range.getWidth() >= 0.1f);
    }
}

// =============================================================================
// Enable/Disable
// =============================================================================

TEST_CASE("AdaptiveRange - enable/disable", "[adaptive-range][enable]") {
    SECTION("Disabled range does not update") {
        AdaptiveRange range({0.3f, 0.7f}, 0.1f);
        range.setEnabled(false);

        float originalMin = range.getMin();
        float originalMax = range.getMax();

        // Try to update with extreme values
        range.update(0.0f);
        range.update(1.0f);

        REQUIRE_THAT(range.getMin(),
                     Catch::Matchers::WithinAbs(originalMin, 0.001f));
        REQUIRE_THAT(range.getMax(),
                     Catch::Matchers::WithinAbs(originalMax, 0.001f));
    }

    SECTION("Re-enabling allows updates") {
        AdaptiveRange range({0.3f, 0.7f}, 0.1f);

        range.setEnabled(false);
        range.update(0.9f);  // Ignored

        range.setEnabled(true);
        range.update(0.9f);  // Should work

        REQUIRE(range.getMax() > 0.7f);
    }

    SECTION("isEnabled reflects state") {
        AdaptiveRange range({0.0f, 1.0f}, 0.1f);

        REQUIRE(range.isEnabled() == true);

        range.setEnabled(false);
        REQUIRE(range.isEnabled() == false);

        range.setEnabled(true);
        REQUIRE(range.isEnabled() == true);
    }
}

// =============================================================================
// Lock
// =============================================================================

TEST_CASE("AdaptiveRange - lock", "[adaptive-range][lock]") {
    SECTION("Locked range does not update") {
        AdaptiveRange range({0.3f, 0.7f}, 0.1f);
        range.setLocked(true);

        float originalMin = range.getMin();
        float originalMax = range.getMax();

        range.update(0.0f);
        range.update(1.0f);

        REQUIRE_THAT(range.getMin(),
                     Catch::Matchers::WithinAbs(originalMin, 0.001f));
        REQUIRE_THAT(range.getMax(),
                     Catch::Matchers::WithinAbs(originalMax, 0.001f));
    }

    SECTION("Unlocking allows updates") {
        AdaptiveRange range({0.3f, 0.7f}, 0.1f);

        range.setLocked(true);
        range.update(0.9f);  // Ignored

        range.setLocked(false);
        range.update(0.9f);  // Should work

        REQUIRE(range.getMax() > 0.7f);
    }

    SECTION("isLocked reflects state") {
        AdaptiveRange range({0.0f, 1.0f}, 0.1f);

        REQUIRE(range.isLocked() == false);

        range.setLocked(true);
        REQUIRE(range.isLocked() == true);

        range.setLocked(false);
        REQUIRE(range.isLocked() == false);
    }

    SECTION("Lock takes precedence even when enabled") {
        AdaptiveRange range({0.3f, 0.7f}, 0.1f);

        REQUIRE(range.isEnabled() == true);
        range.setLocked(true);

        float originalMax = range.getMax();
        range.update(0.9f);

        REQUIRE_THAT(range.getMax(),
                     Catch::Matchers::WithinAbs(originalMax, 0.001f));
    }
}

// =============================================================================
// Direct Bounds Manipulation
// =============================================================================

TEST_CASE("AdaptiveRange - setBounds", "[adaptive-range][bounds]") {
    SECTION("setBounds directly sets min and max") {
        AdaptiveRange range({0.0f, 1.0f}, 0.1f);

        range.setBounds({0.25f, 0.75f});

        REQUIRE_THAT(range.getMin(), Catch::Matchers::WithinAbs(0.25f, 0.001f));
        REQUIRE_THAT(range.getMax(), Catch::Matchers::WithinAbs(0.75f, 0.001f));
    }

    SECTION("setBounds works even when locked") {
        AdaptiveRange range({0.0f, 1.0f}, 0.1f);
        range.setLocked(true);

        range.setBounds({0.25f, 0.75f});

        REQUIRE_THAT(range.getMin(), Catch::Matchers::WithinAbs(0.25f, 0.001f));
        REQUIRE_THAT(range.getMax(), Catch::Matchers::WithinAbs(0.75f, 0.001f));
    }

    SECTION("setBounds works even when disabled") {
        AdaptiveRange range({0.0f, 1.0f}, 0.1f);
        range.setEnabled(false);

        range.setBounds({0.25f, 0.75f});

        REQUIRE_THAT(range.getMin(), Catch::Matchers::WithinAbs(0.25f, 0.001f));
        REQUIRE_THAT(range.getMax(), Catch::Matchers::WithinAbs(0.75f, 0.001f));
    }
}

// =============================================================================
// Adaptation Rate
// =============================================================================

TEST_CASE("AdaptiveRange - adaptation rate", "[adaptive-range][rate]") {
    SECTION("setAdaptationRate changes rate") {
        AdaptiveRange range({0.0f, 1.0f}, 0.01f);

        range.setAdaptationRate(0.05f);

        REQUIRE_THAT(range.getAdaptationRate(),
                     Catch::Matchers::WithinAbs(0.05f, 0.0001f));
    }

    SECTION("Higher rate means faster adaptation") {
        AdaptiveRange slowRange({0.4f, 0.6f}, 0.01f);
        AdaptiveRange fastRange({0.4f, 0.6f}, 0.1f);

        // Feed same values to both
        for (int i = 0; i < 10; ++i) {
            slowRange.update(0.9f);
            fastRange.update(0.9f);
        }

        // Fast range should have expanded more
        REQUIRE(fastRange.getMax() > slowRange.getMax());
    }

    SECTION("Lower rate means slower adaptation") {
        // Use wider starting range (0.4) to avoid minSeparation (0.3) dominating
        AdaptiveRange slowRange({0.3f, 0.7f}, 0.001f);
        AdaptiveRange fastRange({0.3f, 0.7f}, 0.1f);

        for (int i = 0; i < 10; ++i) {
            slowRange.update(0.9f);
            fastRange.update(0.9f);
        }

        float slowDelta = slowRange.getMax() - 0.7f;
        float fastDelta = fastRange.getMax() - 0.7f;

        // Fast should adapt more than slow (exact ratio depends on implementation)
        REQUIRE(fastDelta > slowDelta);
    }
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_CASE("AdaptiveRange - edge cases", "[adaptive-range][edge]") {
    SECTION("Handles zero-width initial range") {
        AdaptiveRange range({0.5f, 0.5f}, 0.1f, 0.05f);

        // After update, should have at least minimum separation
        range.update(0.5f);
        REQUIRE(range.getWidth() >= 0.05f - 0.0001f);
    }

    SECTION("Handles inverted initial range") {
        // Max < Min initially - still works, min separation enforced
        AdaptiveRange range({0.7f, 0.3f}, 0.1f, 0.1f);

        range.update(0.5f);

        // Should enforce minimum separation
        REQUIRE(range.getWidth() >= 0.1f);
    }

    SECTION("Handles extreme values") {
        AdaptiveRange range({0.3f, 0.7f}, 0.1f);

        range.update(-100.0f);  // Very low
        range.update(100.0f);   // Very high

        // Should still have valid range
        REQUIRE(range.getMin() < range.getMax());
    }

    SECTION("Stable under repeated identical updates") {
        AdaptiveRange range({0.2f, 0.8f}, 0.1f);

        // Many identical updates should stabilize
        for (int i = 0; i < 1000; ++i) {
            range.update(0.5f);
        }

        // Should be stable (not NaN or Inf)
        REQUIRE(std::isfinite(range.getMin()));
        REQUIRE(std::isfinite(range.getMax()));
        REQUIRE(range.getWidth() >= 0.3f - 0.0001f);  // Default minSeparation
    }
}

// =============================================================================
// Bounds Clamping (prevents negative min or max > 1)
// =============================================================================

TEST_CASE("AdaptiveRange - bounds clamping", "[adaptive-range][clamping]") {
    SECTION("Min clamped to validMin when centering would go negative") {
        // With repeated zero input, the range contracts toward 0.
        // minSeparation centering would push min negative, but clamping prevents it.
        AdaptiveRange range({0.0f, 0.5f}, 0.1f, 0.3f);

        // Feed many zero values to contract range toward low end
        for (int i = 0; i < 500; ++i) {
            range.update(0.0f);
        }

        // Min should be clamped to validMin (0), not negative
        REQUIRE(range.getMin() >= AdaptiveRange::validMin);
        // Max should be at least minSeparation above min
        REQUIRE(range.getMax() >= range.getMin() + 0.3f - 0.001f);
    }

    SECTION("Max clamped to validMax when centering would exceed 1") {
        // With repeated high input, range expands toward 1.
        // minSeparation centering would push max > 1, but clamping prevents it.
        AdaptiveRange range({0.5f, 1.0f}, 0.1f, 0.3f);

        // Feed many 1.0 values to contract range toward high end
        for (int i = 0; i < 500; ++i) {
            range.update(1.0f);
        }

        // Max should be clamped to validMax (1), not exceed it
        REQUIRE(range.getMax() <= AdaptiveRange::validMax);
        // Min should be at least minSeparation below max
        REQUIRE(range.getMin() <= range.getMax() - 0.3f + 0.001f);
    }

    SECTION("Centered expansion when room on both sides") {
        // When center is ~0.5, expansion should be symmetric
        AdaptiveRange range({0.48f, 0.52f}, 0.1f, 0.3f);

        range.update(0.5f);

        // Should expand symmetrically around ~0.5
        float center = range.getCenter();
        REQUIRE_THAT(center, Catch::Matchers::WithinAbs(0.5f, 0.05f));
        // Both bounds should be within valid range
        REQUIRE(range.getMin() >= AdaptiveRange::validMin);
        REQUIRE(range.getMax() <= AdaptiveRange::validMax);
    }

    SECTION("Zero input produces zero output after clamping fix") {
        // This tests the specific bug that was fixed:
        // With constant zero loudness, min should clamp to 0 (not go negative)
        // so that jmap(0, 0, max, 0, 1) = 0 (instead of ~0.136 with negative min)
        AdaptiveRange range({0.0f, 1.0f}, 0.01f, 0.3f);

        // Simulate many zero-loudness updates
        for (int i = 0; i < 1000; ++i) {
            range.update(0.0f);
        }

        // Min must be exactly 0 (clamped), not negative - THIS IS THE KEY FIX
        REQUIRE_THAT(range.getMin(), Catch::Matchers::WithinAbs(0.0f, 0.001f));
        // Max should be at least minSeparation above min to maintain separation
        REQUIRE(range.getMax() >= 0.3f - 0.001f);
    }
}
