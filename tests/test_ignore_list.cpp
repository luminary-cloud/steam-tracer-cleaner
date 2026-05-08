#include <doctest/doctest.h>

#include "core/ignore_list.hpp"

using stc::core::IgnoreList;

TEST_CASE("preserves_account exact match") {
    IgnoreList l;
    l.preserved_account_ids = {L"76561198000000001"};
    CHECK(l.preserves_account(L"76561198000000001"));
    CHECK_FALSE(l.preserves_account(L"76561198000000002"));
    CHECK_FALSE(l.preserves_account(L""));
}

TEST_CASE("preserves_path is case-insensitive prefix") {
    IgnoreList l;
    l.preserved_paths = {L"C:\\Program Files (x86)\\Steam\\config\\custom"};
    CHECK(l.preserves_path(L"C:\\Program Files (x86)\\Steam\\config\\custom\\settings.vdf"));
    CHECK(l.preserves_path(L"c:\\program files (x86)\\steam\\config\\custom"));
    CHECK_FALSE(l.preserves_path(L"C:\\Program Files (x86)\\Steam\\config\\loginusers.vdf"));
}

TEST_CASE("preserves_registry supports prefix and KEY::VALUE forms") {
    IgnoreList l;
    l.preserved_registry_values = {
        L"HKCU\\Software\\Valve\\Steam\\Users\\76561198000000001",
        L"HKCU\\Software\\Valve\\Steam::AutoLoginUser",
    };
    CHECK(l.preserves_registry(L"HKCU\\Software\\Valve\\Steam\\Users\\76561198000000001\\Sub", L""));
    CHECK(l.preserves_registry(L"HKCU\\Software\\Valve\\Steam", L"AutoLoginUser"));
    CHECK_FALSE(l.preserves_registry(L"HKCU\\Software\\Valve\\Steam", L"RememberPassword"));
}
