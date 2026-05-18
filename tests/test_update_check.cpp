#include <doctest/doctest.h>

#include "core/update_check.hpp"

using stc::core::update_check::semver_less;

TEST_CASE("semver_less compares major.minor.patch") {
    CHECK(semver_less("0.1.0", "0.1.1"));
    CHECK(semver_less("0.1.9", "0.2.0"));
    CHECK(semver_less("0.9.9", "1.0.0"));
    CHECK_FALSE(semver_less("0.1.1", "0.1.0"));
    CHECK_FALSE(semver_less("1.0.0", "0.9.9"));
    CHECK_FALSE(semver_less("0.1.0", "0.1.0"));
}

TEST_CASE("semver_less strips a leading 'v'") {
    CHECK(semver_less("v0.1.0", "0.2.0"));
    CHECK(semver_less("0.1.0", "v0.2.0"));
    CHECK(semver_less("v0.1.0", "v0.1.1"));
    CHECK_FALSE(semver_less("v0.2.0", "v0.1.0"));
}

TEST_CASE("semver_less returns false on garbage input") {
    CHECK_FALSE(semver_less("garbage", "0.1.0"));
    CHECK_FALSE(semver_less("0.1.0", "garbage"));
    CHECK_FALSE(semver_less("", "0.1.0"));
    CHECK_FALSE(semver_less("0.1", "0.2.0"));
}

TEST_CASE("semver_less ignores trailing segments beyond major.minor.patch") {
    CHECK(semver_less("0.1.0.0", "0.2.0"));
    CHECK_FALSE(semver_less("0.1.0.0", "0.1.0"));
}
