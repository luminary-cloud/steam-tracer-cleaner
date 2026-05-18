#include <doctest/doctest.h>

#include <windows.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "core/operation.hpp"
#include "core/targets.hpp"

using stc::core::find_target;
using stc::core::OpKind;
using stc::core::Operation;
using stc::core::ResolveContext;
using stc::core::steam::AccountInfo;
using stc::core::steam::InstallInfo;

namespace fs = std::filesystem;

namespace {

std::atomic<unsigned> g_counter{0};

fs::path make_temp_install() {
    std::string name = "stc_avatar_test_";
    name += std::to_string(::GetCurrentProcessId());
    name += "_";
    name += std::to_string(g_counter.fetch_add(1));
    fs::path base = fs::temp_directory_path() / name;
    fs::create_directories(base / "config" / "avatarcache");
    fs::create_directories(base / "userdata");
    return base;
}

void touch(const fs::path& p) {
    std::ofstream f(p);
    f << "x";
}

}  // namespace

TEST_CASE("resolve_avatarcache tags SteamID64-named files with account_steamid64") {
    auto root = make_temp_install();
    touch(root / "config" / "avatarcache" / "76561198000000001.png");
    touch(root / "config" / "avatarcache" / "76561198000000002.png");
    touch(root / "config" / "avatarcache" / "friendhash_abcdef.jpg");
    touch(root / "config" / "avatarcache" / "notasteamid.png");

    InstallInfo install;
    install.install_path = root;
    install.config_dir = root / "config";
    install.userdata_dir = root / "userdata";
    install.appcache_dir = root / "appcache";

    std::vector<AccountInfo> accounts;
    std::vector<fs::path> libraries;
    ResolveContext ctx{install, accounts, libraries};

    const auto* target = find_target("steam.avatarcache");
    REQUIRE(target != nullptr);
    auto ops = target->resolve(ctx);

    int tagged = 0;
    int untagged = 0;
    for (const auto& op : ops) {
        CHECK(op.kind == OpKind::RemoveFile);
        if (!op.account_steamid64.empty()) {
            ++tagged;
            CHECK((op.account_steamid64 == L"76561198000000001" ||
                   op.account_steamid64 == L"76561198000000002"));
        } else {
            ++untagged;
        }
    }
    CHECK(tagged == 2);
    CHECK(untagged == 2);  // friendhash_*.jpg and notasteamid.png

    std::error_code ec;
    fs::remove_all(root, ec);
}

TEST_CASE("resolve_avatarcache leaves account_steamid64 empty for sub-base SteamIDs") {
    // Numbers below 0x0110000100000000 don't decode to a real account id; we shouldn't tag them.
    auto root = make_temp_install();
    touch(root / "config" / "avatarcache" / "12345.png");

    InstallInfo install;
    install.install_path = root;
    install.config_dir = root / "config";
    install.userdata_dir = root / "userdata";
    install.appcache_dir = root / "appcache";

    std::vector<AccountInfo> accounts;
    std::vector<fs::path> libraries;
    ResolveContext ctx{install, accounts, libraries};

    const auto* target = find_target("steam.avatarcache");
    REQUIRE(target != nullptr);
    auto ops = target->resolve(ctx);

    REQUIRE(ops.size() == 1);
    CHECK(ops[0].account_steamid64.empty());

    std::error_code ec;
    fs::remove_all(root, ec);
}
