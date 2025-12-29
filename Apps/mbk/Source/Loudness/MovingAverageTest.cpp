#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "Catch2.hpp"
#include "MovingAverage.h"

TEST_CASE("setting period") {
    MovingAverage ma(10);
    REQUIRE(ma.getPeriod() == 10);

    SECTION("setting as maxPeriod yields maxPeriod") {
        ma.setPeriod(32);
        REQUIRE(ma.getPeriod() == 32);
    }

    SECTION("setting as above maxPeriod yields maxPeriod") {
        ma.setPeriod(320);
        REQUIRE(ma.getPeriod() == 32);
    }

    SECTION("setting as 0 yields period of 1") {
        ma.setPeriod(0);
        REQUIRE(ma.getPeriod() == 1);
    }

    SECTION("setting as minimum yields minimum") {
        ma.setPeriod(1);
        REQUIRE(ma.getPeriod() == 1);
    }

    SECTION("setting as valid mid-range yields same") {
        ma.setPeriod(15);
        REQUIRE(ma.getPeriod() == 15);
    }
}

TEST_CASE("all zeroes always produces zeroes") {
    MovingAverage ma(10);

    for (int i = -1; i <= 33; i++) {
        ma.setPeriod(i);
        for (int i = 0; i < 64; i++) {
            ma.add(0.0f);
            REQUIRE(ma.getAverage() == 0.0f);
        }
    }
}

TEST_CASE("same value always produces same value once full") {
    for (int i = -1; i <= 33; i++) {
        SECTION(std::to_string(i)) {
            MovingAverage ma(i);
            for (int i = 0; i < 32; i++) {
                ma.add(100.0f);
            }
            REQUIRE(ma.getAverage() == 100.0f);
        }
    }
}

TEST_CASE("half full produces half of constant input") {
    for (int i = 2; i <= 32; i += 2) {
        SECTION(std::to_string(i)) {
            MovingAverage ma(i);
            for (int j = 0; j < i / 2; j++) {
                ma.add(50.0f);
            }
            REQUIRE(ma.getAverage() == 25.0f);
        }
    }
}

TEST_CASE("small period wraps correctly after many additions") {
    // This test catches the bug where index wrap-around used _period instead of maxWindow.
    // With period=4 and maxWindow=32, after adding 64 values the internal _index wraps
    // around the full buffer. The getAverage() must still correctly access the last
    // _period values.
    MovingAverage ma(4);

    // Fill with zeros first (more than maxWindow to ensure full wrap)
    for (int i = 0; i < 64; i++) {
        ma.add(0.0f);
    }
    REQUIRE(ma.getAverage() == 0.0f);

    // Now add 4 values of 100 - the average should be 100
    for (int i = 0; i < 4; i++) {
        ma.add(100.0f);
    }
    REQUIRE(ma.getAverage() == 100.0f);

    // Add 2 more values of 0 - average of last 4 should be 50
    ma.add(0.0f);
    ma.add(0.0f);
    REQUIRE(ma.getAverage() == 50.0f);
}

TEST_CASE("wrap-around works for all period sizes after buffer overflow") {
    // Ensure wrap-around works correctly for various period sizes after
    // the internal index has wrapped around the full maxWindow buffer
    for (int period = 1; period <= 32; period++) {
        SECTION("period " + std::to_string(period)) {
            MovingAverage ma(period);

            // Add enough values to wrap around maxWindow multiple times
            for (int i = 0; i < 100; i++) {
                ma.add(0.0f);
            }

            // Now add 'period' values of 10.0
            for (int i = 0; i < period; i++) {
                ma.add(10.0f);
            }

            REQUIRE(ma.getAverage() == 10.0f);
        }
    }
}