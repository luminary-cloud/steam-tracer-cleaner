#include "ui/widgets/target_tree.hpp"

#include <imgui.h>

#include <map>

#include "ui/util.hpp"

namespace stc::ui::widgets {
namespace {

const char* category_label(stc::core::TargetCategory c) {
    switch (c) {
        case stc::core::TargetCategory::Cache: return "Caches";
        case stc::core::TargetCategory::Log: return "Logs";
        case stc::core::TargetCategory::AccountResidue: return "Account residue";
        case stc::core::TargetCategory::BrowserResidue: return "Browser residue";
        case stc::core::TargetCategory::ControllerResidue: return "Controller bindings";
        case stc::core::TargetCategory::GameData: return "Game data (userdata)";
        case stc::core::TargetCategory::CrashDump: return "Crash dumps";
        case stc::core::TargetCategory::Hwid: return "Hardware identifiers (audit only)";
    }
    return "Other";
}

}  // namespace

bool target_tree(std::set<std::string>& selected) {
    bool changed = false;

    std::map<stc::core::TargetCategory, std::vector<const stc::core::Target*>> grouped;
    for (const auto& t : stc::core::all_targets()) {
        grouped[t.category].push_back(&t);
    }

    constexpr int kColumns = 3;

    for (auto& [cat, items] : grouped) {
        if (ImGui::CollapsingHeader(category_label(cat), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            std::string table_id = std::string{"##tt_"} + category_label(cat);
            if (ImGui::BeginTable(table_id.c_str(), kColumns,
                                  ImGuiTableFlags_SizingStretchSame |
                                      ImGuiTableFlags_NoSavedSettings)) {
                for (const auto* t : items) {
                    ImGui::TableNextColumn();
                    bool sel = selected.contains(t->id);
                    std::string label = stc::ui::to_utf8(t->display_name);
                    if (ImGui::Checkbox((label + "##" + t->id).c_str(), &sel)) {
                        if (sel) {
                            selected.insert(t->id);
                        } else {
                            selected.erase(t->id);
                        }
                        changed = true;
                    }
                    if (!t->description.empty() && ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s", stc::ui::to_utf8(t->description).c_str());
                    }
                }
                ImGui::EndTable();
            }
            ImGui::Unindent();
        }
    }

    return changed;
}

}  // namespace stc::ui::widgets
