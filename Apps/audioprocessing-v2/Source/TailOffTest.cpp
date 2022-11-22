#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "Catch2.hpp"
#include "TailOff.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("constructor") {
    SECTION("0 exponent") {
        TailOff to(0);
        REQUIRE(to.getExponent() == 0);
    }

    SECTION("too high exponent") {
        TailOff to(10);
        REQUIRE(to.getExponent() == 0.9999f);
    }

    SECTION("max exponent") {
        TailOff to(0.9999f);
        REQUIRE(to.getExponent() == 0.9999f);
    }

    SECTION("valid exponent") {
        TailOff to(0.5f);
        REQUIRE(to.getExponent() == 0.5f);
    }

    SECTION("negative exponent") {
        TailOff to(-10.0f);
        REQUIRE(to.getExponent() == 0.0f);
    }
}

TEST_CASE("decaying") {
    TailOff to(0.5f);
    SECTION("Initial value of 0.0f") {
        REQUIRE_THAT(to.getValue(0.0f), WithinAbs(0.0f, 0.000001f));
    }

    SECTION("Jump to higher should yield input value") {
        REQUIRE_THAT(to.getValue(10.0f), WithinAbs(10.0f, 0.000001f));
    }

    SECTION("Input of lower than half should yield half of previous") {
        REQUIRE_THAT(to.getValue(0.0f), WithinAbs(5.0f, 0.000001f));
    }

    SECTION("Input of higher than half should yield input") {
        REQUIRE_THAT(to.getValue(8.0f), WithinAbs(8.0f, 0.000001f));
        REQUIRE_THAT(to.getValue(6.0f), WithinAbs(6.0f, 0.000001f));
    }

    SECTION("Decaying over time") {
        REQUIRE_THAT(to.getValue(0.0f), WithinAbs(3.0f, 0.000001f));
        REQUIRE_THAT(to.getValue(0.0f), WithinAbs(1.5f, 0.000001f));
        REQUIRE_THAT(to.getValue(0.0f), WithinAbs(0.75f, 0.000001f));
        REQUIRE_THAT(to.getValue(0.0f), WithinAbs(0.375f, 0.000001f));
        REQUIRE_THAT(to.getValue(0.0f), WithinAbs(0.1875f, 0.000001f));
    }
}
