#include "ui/screens/audit_screen.hpp"

#include <windows.h>

#include <commdlg.h>
#include <imgui.h>

#include <fstream>
#include <optional>

#include "core/audit.hpp"
#include "ui/util.hpp"

namespace stc::ui::screens {
namespace {

std::optional<stc::core::audit::Report>& cached() {
    static std::optional<stc::core::audit::Report> r;
    return r;
}

}  // namespace

void draw_audit_screen(stc::app::AppState& state) {
    if (!state.install) {
        ImGui::TextWrapped("Steam install not detected. Open the Settings tab and click Refresh.");
        return;
    }

    if (!cached()) {
        cached() = stc::core::audit::build_report(*state.install, state.accounts);
    }

    if (ImGui::Button("Refresh##audit")) {
        cached() = stc::core::audit::build_report(*state.install, state.accounts);
    }
    ImGui::SameLine();
    if (ImGui::Button("Export to file...##audit")) {
        wchar_t buf[MAX_PATH] = L"audit-report.txt";
        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = buf;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = L"Text (*.txt)\0*.txt\0All files (*.*)\0*.*\0\0";
        ofn.lpstrTitle = L"Save audit report";
        ofn.lpstrDefExt = L"txt";
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
        if (GetSaveFileNameW(&ofn)) {
            std::ofstream f(buf);
            if (f) {
                f << stc::core::audit::render_text(*cached());
            }
        }
    }

    ImGui::Spacing();
    const auto& r = *cached();

    if (ImGui::CollapsingHeader("Hardware identifiers", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        ImGui::Text("Computer name: %s", stc::ui::to_utf8(r.hwid.computer_name).c_str());
        ImGui::Text("Machine GUID:  %s", stc::ui::to_utf8(r.hwid.machine_guid).c_str());
        ImGui::Text("Machine ID:    %s", stc::ui::to_utf8(r.hwid.machine_id).c_str());
        ImGui::TextDisabled("Read-only. The cleaner does not modify these.");
        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Accounts", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        if (r.accounts.empty()) {
            ImGui::TextDisabled("No accounts found.");
        }
        for (const auto& a : r.accounts) {
            ImGui::Text("%s   %s", stc::ui::to_utf8(a.steamid64).c_str(),
                        stc::ui::to_utf8(a.persona_name).c_str());
            ImGui::Indent();
            ImGui::TextDisabled("login: %s", stc::ui::to_utf8(a.account_name).c_str());
            ImGui::TextDisabled("userdata: %s   localconfig: %s   loginusers: %s",
                                stc::ui::format_bytes(a.userdata_bytes).c_str(),
                                a.has_localconfig ? "present" : "missing",
                                a.has_loginusers_entry ? "present" : "missing");
            ImGui::Unindent();
            ImGui::Spacing();
        }
        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Browser cookie databases", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        if (r.browsers.empty()) {
            ImGui::TextDisabled("None detected.");
        }
        for (const auto& b : r.browsers) {
            ImGui::Text("%s / %s", stc::ui::to_utf8(b.browser).c_str(),
                        stc::ui::to_utf8(b.profile).c_str());
            ImGui::TextDisabled("%s", b.cookies_db.string().c_str());
        }
        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Crash dumps", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        if (r.crash_dumps.empty()) {
            ImGui::TextDisabled("None.");
        }
        for (const auto& d : r.crash_dumps) {
            ImGui::Text("%s", d.dir.string().c_str());
            ImGui::TextDisabled("count=%llu, total=%s", static_cast<unsigned long long>(d.count),
                                stc::ui::format_bytes(d.total_bytes).c_str());
        }
        ImGui::Unindent();
    }
}

}  // namespace stc::ui::screens
