#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "core/vdf.hpp"

namespace vdf = stc::core::vdf;

TEST_CASE("parses a minimal loginusers-shaped document") {
    constexpr wchar_t source[] =
        LR"("users"
{
    "76561198000000001"
    {
        "AccountName" "alice"
        "PersonaName" "Alice"
        "RememberPassword" "1"
        "mostrecent" "1"
    }
    "76561198000000002"
    {
        "AccountName" "bob"
        "PersonaName" "Bob"
        "RememberPassword" "0"
        "mostrecent" "0"
    }
}
)";
    auto doc = vdf::parse(source);
    REQUIRE(doc.has_value());
    CHECK(doc->root_key == L"users");
    REQUIRE(doc->root);
    REQUIRE(doc->root->is_object());
    CHECK(doc->root->children().size() == 2);

    const auto* alice = doc->root->find(L"76561198000000001");
    REQUIRE(alice != nullptr);
    REQUIRE(alice->is_object());
    const auto* alice_login = alice->find(L"AccountName");
    REQUIRE(alice_login != nullptr);
    CHECK(alice_login->value() == L"alice");
}

TEST_CASE("removes a child from the root object") {
    constexpr wchar_t source[] =
        LR"("users"
{
    "76561198000000001" { "AccountName" "alice" }
    "76561198000000002" { "AccountName" "bob" }
}
)";
    auto doc = vdf::parse(source);
    REQUIRE(doc.has_value());
    CHECK(doc->root->remove(L"76561198000000002"));
    CHECK(doc->root->children().size() == 1);
    CHECK(doc->root->find(L"76561198000000002") == nullptr);
}

TEST_CASE("round trips through serialize") {
    constexpr wchar_t source[] =
        LR"("InstallConfigStore"
{
    "Software"
    {
        "Valve"
        {
            "Steam" { "Language" "english" }
        }
    }
}
)";
    auto doc = vdf::parse(source);
    REQUIRE(doc.has_value());
    auto out = vdf::serialize(*doc);
    auto reparsed = vdf::parse(out);
    REQUIRE(reparsed.has_value());
    const auto* steam = reparsed->root->find(L"Software")
                                       ->find(L"Valve")
                                       ->find(L"Steam");
    REQUIRE(steam != nullptr);
    const auto* lang = steam->find(L"Language");
    REQUIRE(lang != nullptr);
    CHECK(lang->value() == L"english");
}

TEST_CASE("tolerates // line comments and conditional brackets") {
    constexpr wchar_t source[] =
        LR"("Test"
{
    // a comment
    "key" "value" [$WIN32]
}
)";
    auto doc = vdf::parse(source);
    REQUIRE(doc.has_value());
    const auto* k = doc->root->find(L"key");
    REQUIRE(k != nullptr);
    CHECK(k->value() == L"value");
}

TEST_CASE("handles escaped characters in strings") {
    constexpr wchar_t source[] =
        LR"("Test"
{
    "path" "C:\\Program Files\\Steam"
    "quote" "He said \"hi\""
}
)";
    auto doc = vdf::parse(source);
    REQUIRE(doc.has_value());
    CHECK(doc->root->find(L"path")->value() == LR"(C:\Program Files\Steam)");
    CHECK(doc->root->find(L"quote")->value() == LR"(He said "hi")");
}
