#include "ui/widgets/account_picker.hpp"

#include <imgui.h>

#include "ui/util.hpp"

namespace stc::ui::widgets {

bool account_picker(const std::vector<stc::core::steam::AccountInfo>& accounts,
                    std::set<std::uint32_t>& selected_account_ids) {
    bool changed = false;

    if (accounts.empty()) {
        ImGui::TextDisabled("No accounts found in Steam userdata.");
        return false;
    }

    for (const auto& acc : accounts) {
        bool sel = selected_account_ids.contains(acc.account_id);
        std::string persona =
            acc.persona_name.empty() ? "(no persona)" : stc::ui::to_utf8(acc.persona_name);
        std::string login = stc::ui::to_utf8(acc.account_name);
        std::string sid = stc::ui::to_utf8(acc.steamid64);
        std::string label = persona + "  -  " + login + "  -  " + sid + "##acc" + sid;
        if (ImGui::Checkbox(label.c_str(), &sel)) {
            if (sel) {
                selected_account_ids.insert(acc.account_id);
            } else {
                selected_account_ids.erase(acc.account_id);
            }
            changed = true;
        }
    }
    return changed;
}

}  // namespace stc::ui::widgets
