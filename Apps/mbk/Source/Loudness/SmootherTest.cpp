#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "Catch2.hpp"
#include "Smoother.h"

using namespace Loudness;

TEST_CASE("setting smoothing") {
    Smoother s(0.5f);
    REQUIRE(s.getSmoothing() == Catch::Approx(0.5f));

    SECTION("setting to 0 yields 0") {
        s.setSmoothing(0.0f);
        REQUIRE(s.getSmoothing() == Catch::Approx(0.0f));
    }

    SECTION("setting to 1 yields 1") {
        s.setSmoothing(1.0f);
        REQUIRE(s.getSmoothing() == Catch::Approx(1.0f));
    }

    SECTION("setting above 1 clamps to 1") {
        s.setSmoothing(5.0f);
        REQUIRE(s.getSmoothing() == Catch::Approx(1.0f));
    }

    SECTION("setting below 0 clamps to 0") {
        s.setSmoothing(-1.0f);
        REQUIRE(s.getSmoothing() == Catch::Approx(0.0f));
    }

    SECTION("setting to mid-range value works") {
        s.setSmoothing(0.3f);
        REQUIRE(s.getSmoothing() == Catch::Approx(0.3f));
    }
}

TEST_CASE("no smoothing passes values through") {
    Smoother s(0.0f);  // alpha = 1.0, no smoothing

    s.add(0.5f);
    REQUIRE(s.get() == Catch::Approx(0.5f));

    s.add(1.0f);
    REQUIRE(s.get() == Catch::Approx(1.0f));

    s.add(0.0f);
    REQUIRE(s.get() == Catch::Approx(0.0f));
}

TEST_CASE("first value initializes directly") {
    Smoother s(0.9f);  // Heavy smoothing

    // First value should be set directly (no previous average to blend with)
    s.add(0.8f);
    REQUIRE(s.get() == Catch::Approx(0.8f));
}

TEST_CASE("heavy smoothing changes slowly") {
    Smoother s(0.9f);  // alpha ≈ 0.145

    s.add(1.0f);
    REQUIRE(s.get() == Catch::Approx(1.0f));

    // Add a zero - should move slowly towards 0
    s.add(0.0f);
    float afterFirstZero = s.get();
    REQUIRE(afterFirstZero > 0.5f);  // Should still be mostly 1.0

    // Add more zeros - should continue decreasing
    s.add(0.0f);
    float afterSecondZero = s.get();
    REQUIRE(afterSecondZero < afterFirstZero);
    REQUIRE(afterSecondZero > 0.3f);  // Still not close to 0
}

TEST_CASE("constant input converges to that constant") {
    for (float smoothing = 0.1f; smoothing <= 0.9f; smoothing += 0.2f) {
        SECTION("smoothing " + std::to_string(smoothing)) {
            Smoother s(smoothing);

            // Feed constant value many times
            for (int i = 0; i < 100; i++) {
                s.add(0.75f);
            }

            // Should converge to the constant value
            REQUIRE(s.get() == Catch::Approx(0.75f).epsilon(0.001f));
        }
    }
}

TEST_CASE("step response with medium smoothing") {
    Smoother s(0.5f);  // alpha = 0.525

    // Initialize at 0
    s.add(0.0f);
    REQUIRE(s.get() == Catch::Approx(0.0f));

    // Step to 1.0
    s.add(1.0f);
    float v1 = s.get();
    REQUIRE(v1 > 0.4f);   // Should jump significantly
    REQUIRE(v1 < 0.6f);   // But not all the way

    // Continue at 1.0
    s.add(1.0f);
    float v2 = s.get();
    REQUIRE(v2 > v1);     // Should continue increasing

    s.add(1.0f);
    float v3 = s.get();
    REQUIRE(v3 > v2);

    // After enough iterations, should be very close to 1.0
    for (int i = 0; i < 20; i++) {
        s.add(1.0f);
    }
    REQUIRE(s.get() == Catch::Approx(1.0f).epsilon(0.01f));
}

TEST_CASE("all zeroes always produces zeroes") {
    for (float smoothing = 0.0f; smoothing <= 1.0f; smoothing += 0.25f) {
        SECTION("smoothing " + std::to_string(smoothing)) {
            Smoother s(smoothing);

            for (int i = 0; i < 50; i++) {
                s.add(0.0f);
                REQUIRE(s.get() == 0.0f);
            }
        }
    }
}
