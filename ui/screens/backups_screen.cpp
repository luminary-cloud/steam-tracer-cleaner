#include "ui/screens/backups_screen.hpp"

#include <windows.h>

#include <imgui.h>
#include <shellapi.h>

#include "core/backup.hpp"
#include "ui/util.hpp"

namespace stc::ui::screens {
namespace {

void open_in_explorer(const std::filesystem::path& p) {
    ShellExecuteW(nullptr, L"open", L"explorer.exe", (L"/select,\"" + p.wstring() + L"\"").c_str(),
                  nullptr, SW_SHOWNORMAL);
}

}  // namespace

void draw_backups_screen(stc::app::AppState& state) {
    auto entries = stc::core::backup::list_backups(state.backups_dir);

    ImGui::Text("Backups directory: %s", state.backups_dir.string().c_str());
    ImGui::Spacing();

    if (entries.empty()) {
        ImGui::TextDisabled("No backups yet. Backups are created when you click \"Backup and clean\" "
                            "on the Cleaner screen.");
        return;
    }

    if (ImGui::BeginTable("##backups", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Timestamp (UTC)");
        ImGui::TableSetupColumn("Size");
        ImGui::TableSetupColumn("Operations");
        ImGui::TableSetupColumn("Actions");
        ImGui::TableHeadersRow();

        for (const auto& e : entries) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(stc::ui::to_utf8(e.timestamp).c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(stc::ui::format_bytes(e.size_bytes).c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%llu", static_cast<unsigned long long>(e.op_count));
            ImGui::TableSetColumnIndex(3);

            std::string id_suffix = "##" + stc::ui::to_utf8(e.timestamp);

            if (ImGui::SmallButton(("Restore" + id_suffix).c_str())) {
                stc::core::backup::restore(e);
            }
            stc::ui::hover_tooltip("Replay this backup: copy mirrored files back to their original "
                                   "locations and import any exported .reg files.");
            ImGui::SameLine();
            if (ImGui::SmallButton(("Open" + id_suffix).c_str())) {
                open_in_explorer(e.dir);
            }
            stc::ui::hover_tooltip("Open this backup directory in File Explorer.");
            ImGui::SameLine();
            if (ImGui::SmallButton(("Delete" + id_suffix).c_str())) {
                stc::core::backup::remove_backup(e);
            }
            stc::ui::hover_tooltip("Permanently remove this backup directory.");
        }
        ImGui::EndTable();
    }
}

}  // namespace stc::ui::screens
