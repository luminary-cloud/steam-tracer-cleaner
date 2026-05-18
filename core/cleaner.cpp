#include "core/cleaner.hpp"

#include <windows.h>

#include <spdlog/spdlog.h>

#include <filesystem>
#include <utility>

#include "core/browser_cookies.hpp"
#include "core/vdf.hpp"
#include "platform/fs.hpp"
#include "platform/registry.hpp"

namespace stc::core {
namespace {

namespace fs_std = std::filesystem;

struct ParsedRegPath {
    HKEY root = nullptr;
    std::wstring subkey;
};

bool parse_reg_path(std::wstring_view full, ParsedRegPath& out) {
    auto pos = full.find(L'\\');
    if (pos == std::wstring_view::npos) {
        return false;
    }
    auto hive = full.substr(0, pos);
    auto rest = full.substr(pos + 1);
    if (hive == L"HKCU" || hive == L"HKEY_CURRENT_USER") {
        out.root = HKEY_CURRENT_USER;
    } else if (hive == L"HKLM" || hive == L"HKEY_LOCAL_MACHINE") {
        out.root = HKEY_LOCAL_MACHINE;
    } else if (hive == L"HKCR" || hive == L"HKEY_CLASSES_ROOT") {
        out.root = HKEY_CLASSES_ROOT;
    } else if (hive == L"HKU" || hive == L"HKEY_USERS") {
        out.root = HKEY_USERS;
    } else {
        return false;
    }
    out.subkey = std::wstring{rest};
    return true;
}

bool execute_one(const PlanStep& step, backup::Session* bkp, std::wstring& err_out) {
    const auto& op = step.op;
    switch (op.kind) {
        case OpKind::RemoveFile: {
            fs_std::path p{op.target};
            if (bkp) {
                bkp->record_file(p);
                bkp->note_op(op);
            }
            if (!stc::platform::fsx::delete_file(p)) {
                err_out = L"DeleteFile failed: " + p.wstring();
                return false;
            }
            return true;
        }
        case OpKind::RemoveTree: {
            fs_std::path p{op.target};
            if (bkp) {
                bkp->record_directory(p);
                bkp->note_op(op);
            }
            if (!stc::platform::fsx::delete_directory_recursive(p)) {
                err_out = L"DeleteDirectory failed: " + p.wstring();
                return false;
            }
            return true;
        }
        case OpKind::RemoveRegistryValue: {
            ParsedRegPath rp;
            if (!parse_reg_path(op.target, rp)) {
                err_out = L"Invalid registry path: " + op.target;
                return false;
            }
            if (bkp) {
                bkp->record_registry_value(op.target, op.value_name);
                bkp->note_op(op);
            }
            auto r = stc::platform::reg::delete_value(rp.root, rp.subkey, op.value_name);
            if (!r) {
                err_out = L"DeleteRegistryValue failed: " + op.target + L" :: " + op.value_name;
                return false;
            }
            return true;
        }
        case OpKind::RemoveRegistryKey: {
            ParsedRegPath rp;
            if (!parse_reg_path(op.target, rp)) {
                err_out = L"Invalid registry path: " + op.target;
                return false;
            }
            if (bkp) {
                bkp->record_registry_key(op.target);
                bkp->note_op(op);
            }
            auto r = stc::platform::reg::delete_key_recursive(rp.root, rp.subkey);
            if (!r) {
                err_out = L"DeleteRegistryKey failed: " + op.target;
                return false;
            }
            return true;
        }
        case OpKind::ClearRegistryValue: {
            ParsedRegPath rp;
            if (!parse_reg_path(op.target, rp)) {
                err_out = L"Invalid registry path: " + op.target;
                return false;
            }
            if (bkp) {
                bkp->record_registry_value(op.target, op.value_name);
                bkp->note_op(op);
            }
            auto r = stc::platform::reg::write_string(rp.root, rp.subkey, op.value_name, L"");
            if (!r) {
                err_out = L"ClearRegistryValue failed: " + op.target + L" :: " + op.value_name;
                return false;
            }
            return true;
        }
        case OpKind::WriteRegistryString: {
            ParsedRegPath rp;
            if (!parse_reg_path(op.target, rp)) {
                err_out = L"Invalid registry path: " + op.target;
                return false;
            }
            if (bkp) {
                bkp->record_registry_value(op.target, op.value_name);
                bkp->note_op(op);
            }
            auto r = stc::platform::reg::write_string(rp.root, rp.subkey, op.value_name, op.payload);
            if (!r) {
                err_out = L"WriteRegistryString failed: " + op.target + L" :: " + op.value_name;
                return false;
            }
            return true;
        }
        case OpKind::VdfRemoveChild: {
            fs_std::path p{op.target};
            if (bkp) {
                bkp->record_vdf(p);
                bkp->note_op(op);
            }
            auto doc = vdf::load(p);
            if (!doc) {
                err_out = L"VDF parse failed: " + p.wstring();
                return false;
            }
            if (!doc->root || !doc->root->is_object()) {
                err_out = L"VDF root is not an object: " + p.wstring();
                return false;
            }
            // Quick fix: walk the path manually. Was a recursive helper but I couldn't be
            // bothered to add it to vdf.hpp for one caller. value_name is a backslash-separated
            // path; loginusers.vdf passes a single key (no backslash) and falls through fine.
            auto* node = doc->root.get();
            std::wstring remaining = op.value_name;
            while (true) {
                auto sep = remaining.find(L'\\');
                if (sep == std::wstring::npos) {
                    node->remove(remaining);
                    break;
                }
                auto* next = node->find(remaining.substr(0, sep));
                if (!next || !next->is_object()) {
                    return true;  // path doesn't exist; nothing to do
                }
                node = next;
                remaining = remaining.substr(sep + 1);  // intentionally wasteful, gets called <10 times
            }
            if (!vdf::save(*doc, p)) {
                err_out = L"VDF save failed: " + p.wstring();
                return false;
            }
            return true;
        }
        case OpKind::VdfSetValue: {
            fs_std::path p{op.target};
            if (bkp) {
                bkp->record_vdf(p);
                bkp->note_op(op);
            }
            auto doc = vdf::load(p);
            if (!doc) {
                err_out = L"VDF parse failed: " + p.wstring();
                return false;
            }
            if (!doc->root || !doc->root->is_object()) {
                err_out = L"VDF root is not an object: " + p.wstring();
                return false;
            }
            // Missing intermediate segments are treated as a no-op rather than synthesised:
            // Steam rewrites mostrecent on next login, so guessing wrong is worse than nothing.
            auto* node = doc->root.get();
            std::wstring remaining = op.value_name;
            while (true) {
                auto sep = remaining.find(L'\\');
                if (sep == std::wstring::npos) {
                    node->set(remaining, std::make_unique<vdf::Node>(op.payload));
                    break;
                }
                auto* next = node->find(remaining.substr(0, sep));
                if (!next || !next->is_object()) {
                    spdlog::warn("VdfSetValue: path segment missing in {}", p.string());
                    return true;
                }
                node = next;
                remaining = remaining.substr(sep + 1);
            }
            if (!vdf::save(*doc, p)) {
                err_out = L"VDF save failed: " + p.wstring();
                return false;
            }
            return true;
        }
        case OpKind::ClearBrowserSteamCookies: {
            fs_std::path p{op.target};
            if (bkp) {
                bkp->record_file(p);
                bkp->note_op(op);
            }
            auto schema = op.value_name == L"firefox" ? stc::core::browser::Schema::Firefox
                                                      : stc::core::browser::Schema::Chromium;
            int removed = stc::core::browser::clear_steam_cookies(p, schema);
            if (removed < 0) {
                err_out = L"Browser cookie DB locked or unreadable: " + p.wstring();
                return false;
            }
            spdlog::info("Cleared {} Steam cookies from {}", removed, p.string());
            return true;
        }
    }
    err_out = L"Unknown OpKind";
    return false;
}

}  // namespace

CleanResult execute(const Plan& plan, const CleanOptions& opts) {
    CleanResult result;
    int total = static_cast<int>(plan.steps.size());
    int done = 0;

    for (const auto& step : plan.steps) {
        std::wstring err;
        if (execute_one(step, opts.backup, err)) {
            ++result.succeeded;
            result.bytes_freed += step.op.size_bytes;
            spdlog::debug("OK [{}] {}", step.target_id, std::filesystem::path{step.op.target}.string());
        } else {
            ++result.failed;
            result.failure_messages.push_back(err);
            spdlog::warn("FAIL [{}] {}", step.target_id,
                         std::filesystem::path{step.op.target}.string());
        }
        ++done;
        if (opts.on_progress) {
            opts.on_progress(done, total);
        }
    }

    if (opts.backup) {
        opts.backup->finalize();
    }
    return result;
}

}  // namespace stc::core
