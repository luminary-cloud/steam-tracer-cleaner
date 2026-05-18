#include "core/update_check.hpp"

#include <windows.h>

#include <winhttp.h>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <array>
#include <charconv>
#include <string>
#include <vector>

#include "core/version.hpp"

namespace stc::core::update_check {
namespace {

constexpr DWORD kConnectTimeoutMs = 3000;
constexpr DWORD kSendTimeoutMs = 5000;
constexpr DWORD kReceiveTimeoutMs = 5000;
constexpr std::size_t kMaxBodyBytes = 64 * 1024;

struct Handle {
    HINTERNET h = nullptr;
    Handle() = default;
    explicit Handle(HINTERNET v) : h(v) {}
    ~Handle() { if (h) WinHttpCloseHandle(h); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    operator bool() const { return h != nullptr; }
};

std::wstring widen(std::string_view s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring out(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}

std::optional<std::array<int, 3>> parse_semver(std::string_view s) {
    if (!s.empty() && (s.front() == 'v' || s.front() == 'V')) {
        s.remove_prefix(1);
    }
    std::array<int, 3> out{0, 0, 0};
    std::size_t pos = 0;
    for (int i = 0; i < 3; ++i) {
        std::size_t start = pos;
        while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') {
            ++pos;
        }
        if (start == pos) {
            return std::nullopt;
        }
        int v = 0;
        auto [_, ec] = std::from_chars(s.data() + start, s.data() + pos, v);
        if (ec != std::errc{}) {
            return std::nullopt;
        }
        out[i] = v;
        if (i < 2) {
            if (pos >= s.size() || s[pos] != '.') {
                return std::nullopt;
            }
            ++pos;
        }
    }
    return out;
}

}  // namespace

bool semver_less(std::string_view lhs, std::string_view rhs) {
    auto a = parse_semver(lhs);
    auto b = parse_semver(rhs);
    if (!a || !b) {
        return false;
    }
    return *a < *b;
}

std::optional<Result> fetch_latest_release(std::string_view owner_repo,
                                           std::string_view current_version) {
    std::string user_agent_narrow = "steam-tracer-cleaner/";
    user_agent_narrow += stc::core::kAppVersion;
    std::wstring user_agent = widen(user_agent_narrow);

    Handle session{WinHttpOpen(user_agent.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                               WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)};
    if (!session) {
        spdlog::warn("update_check: WinHttpOpen failed: {}", GetLastError());
        return std::nullopt;
    }
    WinHttpSetTimeouts(session.h, kConnectTimeoutMs, kConnectTimeoutMs, kSendTimeoutMs,
                       kReceiveTimeoutMs);

    Handle connect{WinHttpConnect(session.h, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0)};
    if (!connect) {
        spdlog::warn("update_check: WinHttpConnect failed: {}", GetLastError());
        return std::nullopt;
    }

    std::wstring path = L"/repos/" + widen(owner_repo) + L"/releases/latest";
    Handle req{WinHttpOpenRequest(connect.h, L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                  WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)};
    if (!req) {
        spdlog::warn("update_check: WinHttpOpenRequest failed: {}", GetLastError());
        return std::nullopt;
    }

    const wchar_t* headers = L"Accept: application/vnd.github+json\r\n"
                             L"X-GitHub-Api-Version: 2022-11-28\r\n";
    if (!WinHttpSendRequest(req.h, headers, static_cast<DWORD>(-1), WINHTTP_NO_REQUEST_DATA, 0, 0,
                            0)) {
        spdlog::warn("update_check: WinHttpSendRequest failed: {}", GetLastError());
        return std::nullopt;
    }
    if (!WinHttpReceiveResponse(req.h, nullptr)) {
        spdlog::warn("update_check: WinHttpReceiveResponse failed: {}", GetLastError());
        return std::nullopt;
    }

    DWORD status = 0;
    DWORD sz = sizeof(status);
    if (!WinHttpQueryHeaders(req.h, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz, WINHTTP_NO_HEADER_INDEX)) {
        spdlog::warn("update_check: WinHttpQueryHeaders failed: {}", GetLastError());
        return std::nullopt;
    }
    if (status != 200) {
        spdlog::warn("update_check: HTTP {}", status);
        return std::nullopt;
    }

    std::string body;
    body.reserve(8192);
    std::array<char, 8192> buf{};
    DWORD read = 0;
    while (WinHttpReadData(req.h, buf.data(), static_cast<DWORD>(buf.size()), &read) && read > 0) {
        if (body.size() + read > kMaxBodyBytes) {
            spdlog::warn("update_check: response exceeded {} bytes, aborting", kMaxBodyBytes);
            return std::nullopt;
        }
        body.append(buf.data(), read);
    }

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(body);
    } catch (const std::exception& e) {
        spdlog::warn("update_check: JSON parse failed: {}", e.what());
        return std::nullopt;
    }

    auto it = j.find("tag_name");
    if (it == j.end() || !it->is_string()) {
        spdlog::warn("update_check: response missing tag_name");
        return std::nullopt;
    }

    Result out;
    out.latest_tag = it->get<std::string>();
    out.newer_than_current = semver_less(current_version, out.latest_tag);
    spdlog::info("update_check: latest={} current={} newer={}", out.latest_tag, current_version,
                 out.newer_than_current);
    return out;
}

}  // namespace stc::core::update_check
