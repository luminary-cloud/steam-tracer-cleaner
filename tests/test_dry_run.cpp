#include <doctest/doctest.h>

#include <vector>

#include "core/dry_run.hpp"
#include "core/ignore_list.hpp"
#include "core/steam_paths.hpp"

using stc::core::IgnoreList;
using stc::core::pick_autologin_redirect;
using stc::core::steam::AccountInfo;

namespace {

AccountInfo make_account(std::wstring sid64, std::wstring login_name, bool most_recent = false) {
    AccountInfo a;
    a.steamid64 = std::move(sid64);
    a.account_name = std::move(login_name);
    a.most_recent = most_recent;
    return a;
}

}  // namespace

TEST_CASE("pick_autologin_redirect prefers most_recent preserved account") {
    std::vector<AccountInfo> accounts{
        make_account(L"76561198000000001", L"alice", false),
        make_account(L"76561198000000002", L"bob", true),
        make_account(L"76561198000000003", L"carol", false),
    };
    IgnoreList ignore;
    ignore.preserved_account_ids = {L"76561198000000001", L"76561198000000002"};

    auto pick = pick_autologin_redirect(accounts, ignore);
    REQUIRE(pick.has_value());
    CHECK(pick->account_name == L"bob");
    CHECK(pick->steamid64 == L"76561198000000002");
}

TEST_CASE("pick_autologin_redirect falls back to first preserved with a name") {
    std::vector<AccountInfo> accounts{
        make_account(L"76561198000000001", L"alice", false),
        make_account(L"76561198000000002", L"bob", false),
    };
    IgnoreList ignore;
    ignore.preserved_account_ids = {L"76561198000000001", L"76561198000000002"};

    auto pick = pick_autologin_redirect(accounts, ignore);
    REQUIRE(pick.has_value());
    CHECK(pick->account_name == L"alice");
    CHECK(pick->steamid64 == L"76561198000000001");
}

TEST_CASE("pick_autologin_redirect skips accounts with empty account_name") {
    std::vector<AccountInfo> accounts{
        make_account(L"76561198000000001", L"", true),
        make_account(L"76561198000000002", L"bob", false),
    };
    IgnoreList ignore;
    ignore.preserved_account_ids = {L"76561198000000001", L"76561198000000002"};

    auto pick = pick_autologin_redirect(accounts, ignore);
    REQUIRE(pick.has_value());
    CHECK(pick->account_name == L"bob");
    CHECK(pick->steamid64 == L"76561198000000002");
}

TEST_CASE("pick_autologin_redirect ignores non-preserved most_recent") {
    std::vector<AccountInfo> accounts{
        make_account(L"76561198000000001", L"alice", true),
        make_account(L"76561198000000002", L"bob", false),
    };
    IgnoreList ignore;
    ignore.preserved_account_ids = {L"76561198000000002"};

    auto pick = pick_autologin_redirect(accounts, ignore);
    REQUIRE(pick.has_value());
    CHECK(pick->account_name == L"bob");
    CHECK(pick->steamid64 == L"76561198000000002");
}

TEST_CASE("pick_autologin_redirect returns nullopt when no preserved accounts have a name") {
    std::vector<AccountInfo> accounts{
        make_account(L"76561198000000001", L"alice", false),
        make_account(L"76561198000000002", L"", false),
    };
    IgnoreList ignore;
    ignore.preserved_account_ids = {L"76561198000000002"};

    auto pick = pick_autologin_redirect(accounts, ignore);
    CHECK_FALSE(pick.has_value());
}

TEST_CASE("pick_autologin_redirect returns nullopt with no preserved accounts") {
    std::vector<AccountInfo> accounts{
        make_account(L"76561198000000001", L"alice", true),
    };
    IgnoreList ignore;

    auto pick = pick_autologin_redirect(accounts, ignore);
    CHECK_FALSE(pick.has_value());
}

TEST_CASE("pick_autologin_redirect returns nullopt with no accounts at all") {
    std::vector<AccountInfo> accounts;
    IgnoreList ignore;
    ignore.preserved_account_ids = {L"76561198000000001"};

    auto pick = pick_autologin_redirect(accounts, ignore);
    CHECK_FALSE(pick.has_value());
}
