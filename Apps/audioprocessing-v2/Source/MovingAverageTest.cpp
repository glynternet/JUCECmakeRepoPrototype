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