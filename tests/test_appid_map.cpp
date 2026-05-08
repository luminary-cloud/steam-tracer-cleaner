#include <doctest/doctest.h>

#include "core/appid_map.hpp"

namespace appid = stc::core::appid;

TEST_CASE("CS2 entry is correct") {
    const auto* g = appid::find_by_appid(730);
    REQUIRE(g != nullptr);
    CHECK(g->install_folder == L"Counter-Strike Global Offensive");
    CHECK(g->mod_folder == L"game/csgo");
    CHECK(g->is_source2);
}

TEST_CASE("Garry's Mod is Source 1") {
    const auto* g = appid::find_by_appid(4000);
    REQUIRE(g != nullptr);
    CHECK(g->mod_folder == L"garrysmod");
    CHECK_FALSE(g->is_source2);
}

TEST_CASE("Unknown appid returns null") { CHECK(appid::find_by_appid(99999999) == nullptr); }
