#include <doctest/doctest.h>

#include "core/steam_paths.hpp"

namespace steam = stc::core::steam;

TEST_CASE("SteamID64 round-trips with account id") {
    constexpr std::uint32_t aid = 12345678;
    auto sid = steam::account_id_to_steamid64(aid);
    CHECK(steam::steamid64_to_account_id(sid) == aid);
}

TEST_CASE("Invalid SteamID64 yields zero account id") {
    CHECK(steam::steamid64_to_account_id(L"") == 0);
    CHECK(steam::steamid64_to_account_id(L"abc") == 0);
    CHECK(steam::steamid64_to_account_id(L"123") == 0);  // below kSteamIdBase
}
