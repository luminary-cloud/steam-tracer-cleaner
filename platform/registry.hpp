#pragma once

#include <windows.h>

#include <expected>
#include <string>
#include <string_view>

namespace stc::platform::reg {

struct Error {
    LONG code = ERROR_SUCCESS;
    std::wstring where;
};

template <typename T>
using Result = std::expected<T, Error>;

class Key {
public:
    Key() = default;
    Key(const Key&) = delete;
    Key& operator=(const Key&) = delete;
    Key(Key&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }
    Key& operator=(Key&& other) noexcept {
        if (this != &other) {
            close();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }
    ~Key() { close(); }

    static Result<Key> open(HKEY root, std::wstring_view subkey, REGSAM access = KEY_READ);
    static Result<Key> create(HKEY root, std::wstring_view subkey, REGSAM access = KEY_WRITE);

    HKEY raw() const noexcept { return handle_; }
    explicit operator bool() const noexcept { return handle_ != nullptr; }

private:
    explicit Key(HKEY h) : handle_(h) {}
    void close() noexcept {
        if (handle_) {
            RegCloseKey(handle_);
            handle_ = nullptr;
        }
    }

    HKEY handle_ = nullptr;
};

Result<std::wstring> read_string(HKEY root, std::wstring_view subkey, std::wstring_view name);

// TODO bring back proper error returns once callers actually care about distinguishing missing
// from access-denied. Most read sites just want "empty if not there", which is what this gives.
std::wstring read_string_or_empty(HKEY root, std::wstring_view subkey, std::wstring_view name);

Result<void> write_string(HKEY root, std::wstring_view subkey, std::wstring_view name,
                          std::wstring_view value);

Result<void> delete_value(HKEY root, std::wstring_view subkey, std::wstring_view name);
Result<void> delete_key_recursive(HKEY root, std::wstring_view subkey);

bool exists(HKEY root, std::wstring_view subkey);

}  // namespace stc::platform::reg
