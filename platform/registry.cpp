#include "platform/registry.hpp"

#include <spdlog/spdlog.h>

namespace stc::platform::reg {
namespace {

std::wstring to_wstring(std::wstring_view sv) {
    return std::wstring{sv};
}

Error make_error(LONG code, std::wstring_view where) {
    return Error{code, std::wstring{where}};
}

}  // namespace

Result<Key> Key::open(HKEY root, std::wstring_view subkey, REGSAM access) {
    HKEY h = nullptr;
    LONG rc = RegOpenKeyExW(root, to_wstring(subkey).c_str(), 0, access, &h);
    if (rc != ERROR_SUCCESS) {
        return std::unexpected(make_error(rc, L"RegOpenKeyExW"));
    }
    return Key{h};
}

Result<Key> Key::create(HKEY root, std::wstring_view subkey, REGSAM access) {
    HKEY h = nullptr;
    DWORD disp = 0;
    LONG rc = RegCreateKeyExW(root, to_wstring(subkey).c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
                              access, nullptr, &h, &disp);
    if (rc != ERROR_SUCCESS) {
        return std::unexpected(make_error(rc, L"RegCreateKeyExW"));
    }
    return Key{h};
}

Result<std::wstring> read_string(HKEY root, std::wstring_view subkey, std::wstring_view name) {
    auto key = Key::open(root, subkey, KEY_READ);
    if (!key) {
        return std::unexpected(key.error());
    }
    DWORD type = 0;
    DWORD bytes = 0;
    LONG rc = RegQueryValueExW(key->raw(), to_wstring(name).c_str(), nullptr, &type, nullptr, &bytes);
    if (rc != ERROR_SUCCESS) {
        return std::unexpected(make_error(rc, L"RegQueryValueExW(probe)"));
    }
    if (type != REG_SZ && type != REG_EXPAND_SZ) {
        return std::unexpected(make_error(ERROR_DATATYPE_MISMATCH, L"read_string type mismatch"));
    }
    std::wstring buf;
    buf.resize(bytes / sizeof(wchar_t));
    rc = RegQueryValueExW(key->raw(), to_wstring(name).c_str(), nullptr, &type,
                          reinterpret_cast<LPBYTE>(buf.data()), &bytes);
    if (rc != ERROR_SUCCESS) {
        return std::unexpected(make_error(rc, L"RegQueryValueExW(read)"));
    }
    // Trim the trailing NUL the registry includes in `bytes`.
    while (!buf.empty() && buf.back() == L'\0') {
        buf.pop_back();
    }
    return buf;
}

std::wstring read_string_or_empty(HKEY root, std::wstring_view subkey, std::wstring_view name) {
    auto v = read_string(root, subkey, name);
    return v ? *v : std::wstring{};
}

Result<void> write_string(HKEY root, std::wstring_view subkey, std::wstring_view name,
                          std::wstring_view value) {
    auto key = Key::create(root, subkey, KEY_SET_VALUE);
    if (!key) {
        return std::unexpected(key.error());
    }
    auto v = to_wstring(value);
    LONG rc = RegSetValueExW(key->raw(), to_wstring(name).c_str(), 0, REG_SZ,
                             reinterpret_cast<const BYTE*>(v.c_str()),
                             static_cast<DWORD>((v.size() + 1) * sizeof(wchar_t)));
    if (rc != ERROR_SUCCESS) {
        return std::unexpected(make_error(rc, L"RegSetValueExW"));
    }
    return {};
}

Result<void> delete_value(HKEY root, std::wstring_view subkey, std::wstring_view name) {
    auto key = Key::open(root, subkey, KEY_SET_VALUE);
    if (!key) {
        return std::unexpected(key.error());
    }
    LONG rc = RegDeleteValueW(key->raw(), to_wstring(name).c_str());
    if (rc != ERROR_SUCCESS && rc != ERROR_FILE_NOT_FOUND) {
        return std::unexpected(make_error(rc, L"RegDeleteValueW"));
    }
    return {};
}

Result<void> delete_key_recursive(HKEY root, std::wstring_view subkey) {
    LONG rc = RegDeleteTreeW(root, to_wstring(subkey).c_str());
    if (rc != ERROR_SUCCESS && rc != ERROR_FILE_NOT_FOUND) {
        return std::unexpected(make_error(rc, L"RegDeleteTreeW"));
    }
    return {};
}

bool exists(HKEY root, std::wstring_view subkey) {
    return Key::open(root, subkey, KEY_READ).has_value();
}

}  // namespace stc::platform::reg
