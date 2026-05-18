#include "ui/screens/cleaner_screen.hpp"

#include <imgui.h>
#include <spdlog/spdlog.h>

#include <algorithm>

#include "core/cleaner.hpp"
#include "core/dry_run.hpp"
#include "core/steam_lifecycle.hpp"
#include "ui/util.hpp"
#include "ui/widgets/target_tree.hpp"

namespace stc::ui::screens {
namespace {

stc::core::ResolveContext make_ctx(const stc::app::AppState& s) {
    return {*s.install, s.accounts, s.libraries};
}

void apply_profile_selection(stc::app::AppState& state) {
    if (state.profiles.empty()) {
        return;
    }
    state.selected_profile_index =
        std::clamp(state.selected_profile_index, 0, static_cast<int>(state.profiles.size()) - 1);
    const auto& profile = state.profiles[state.selected_profile_index];
    state.selected_target_ids.clear();
    state.selected_target_ids.insert(profile.target_ids.begin(), profile.target_ids.end());
}

void run_dry_run(stc::app::AppState& state) {
    if (!state.install) {
        return;
    }
    auto ctx = make_ctx(state);
    stc::core::PlanOptions opts;
    opts.ignore = &state.ignore_list;
    if (!state.profiles.empty()) {
        const auto& profile = state.profiles[state.selected_profile_index];
        if (profile.name == L"Game Reset") {
            opts.only_appids.assign(state.selected_appids.begin(), state.selected_appids.end());
        }
    }
    std::vector<std::string> ids(state.selected_target_ids.begin(), state.selected_target_ids.end());
    state.last_plan = stc::core::build_plan_by_ids(ids, ctx, opts);
    state.last_result.reset();
}

void run_clean(stc::app::AppState& state, bool with_backup) {
    if (!state.last_plan) {
        run_dry_run(state);
    }
    if (!state.last_plan) {
        return;
    }

    // Steam rewrites loginusers.vdf and localconfig.vdf when it exits, so close it first.
    stc::core::steam_lifecycle::close_steam_and_games();

    std::optional<stc::core::backup::Session> session;
    if (with_backup) {
        session = stc::core::backup::Session::create(state.backups_dir);
    }

    stc::core::CleanOptions opts;
    opts.backup = session ? &*session : nullptr;
    state.last_result = stc::core::execute(*state.last_plan, opts);
    spdlog::info("Cleaner finished: {} ok, {} failed, {} bytes freed",
                 state.last_result->succeeded, state.last_result->failed,
                 state.last_result->bytes_freed);

    if (with_backup && state.backup_keep_count > 0) {
        auto pruned = stc::core::backup::prune_backups(
            state.backups_dir, static_cast<std::size_t>(state.backup_keep_count));
        (void)pruned;  // best effort, refresh button will pick it up next time
    }
}

}  // namespace

void draw_cleaner_screen(stc::app::AppState& state) {
    if (!state.install) {
        ImGui::TextWrapped("Steam install not detected. Open the Settings tab and click Refresh.");
        return;
    }

    ImGui::Text("Profile");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(260);
    int prev = state.selected_profile_index;
    std::vector<const char*> labels;
    std::vector<std::string> label_storage;
    label_storage.reserve(state.profiles.size());
    for (const auto& p : state.profiles) {
        label_storage.push_back(stc::ui::to_utf8(p.name));
    }
    for (auto& s : label_storage) {
        labels.push_back(s.c_str());
    }
    if (ImGui::Combo("##profile", &state.selected_profile_index, labels.data(),
                     static_cast<int>(labels.size()))) {
        if (prev != state.selected_profile_index) {
            apply_profile_selection(state);
        }
    }
    stc::ui::hover_tooltip("Preset bundle of cleanup targets. The list below is the union of every "
                           "target the profile enables.");

    if (!state.profiles.empty()) {
        const auto& profile = state.profiles[state.selected_profile_index];
        if (!profile.description.empty()) {
            ImGui::TextWrapped("%s", stc::ui::to_utf8(profile.description).c_str());
        }
        if (profile.name == L"Game Reset") {
            ImGui::Separator();
            ImGui::Text("Pick AppIDs to reset:");
            ImGui::SameLine();
            ImGui::TextDisabled("(0 = none, common: 730 CS2, 440 TF2, 4000 GMod)");
            static char buf[128] = "730";
            ImGui::SetNextItemWidth(260);
            if (ImGui::InputText("##appids", buf, sizeof(buf))) {
            }
            state.selected_appids.clear();
            std::string s = buf;
            std::size_t i = 0;
            while (i < s.size()) {
                std::uint32_t v = 0;
                while (i < s.size() && (s[i] < '0' || s[i] > '9')) {
                    ++i;
                }
                while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
                    v = v * 10 + static_cast<std::uint32_t>(s[i] - '0');
                    ++i;
                }
                if (v != 0) {
                    state.selected_appids.insert(v);
                }
            }
        }
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Targets");

    // The targets child fills the leftover height. Reservation must match every item drawn below
    // this point exactly, or inner_content grows a scrollbar. Baseline accounts for the Spacing
    // pair around the button row plus the row itself; plan and result branches add their own
    // section heights when present.
    const float spacing = ImGui::GetStyle().ItemSpacing.y;
    const float button_h = 34.0F;
    const float text_h = ImGui::GetTextLineHeight();
    const float plan_list_h = 180.0F;

    float reserved = spacing * 4.0F + button_h;
    if (state.last_plan) {
        reserved += text_h + spacing + plan_list_h;
    }
    if (state.last_result) {
        reserved += spacing + text_h;
    }

    float targets_h = ImGui::GetContentRegionAvail().y - reserved;
    if (targets_h < 200.0F) {
        targets_h = 200.0F;
    }
    ImGui::BeginChild("##targets", ImVec2(0, targets_h), ImGuiChildFlags_Borders);
    stc::ui::widgets::target_tree(state.selected_target_ids);
    ImGui::EndChild();

    ImGui::Spacing();

    if (ImGui::Button("Dry run", ImVec2(120, 34))) {
        run_dry_run(state);
    }
    stc::ui::hover_tooltip(
        "Compute the cleanup plan and list every file, registry value, and VDF entry that "
        "would be touched. Nothing is deleted.");
    ImGui::SameLine();
    if (ImGui::Button("Backup and clean", ImVec2(170, 34))) {
        run_clean(state, true);
    }
    stc::ui::hover_tooltip(
        "Mirror affected files and export affected registry keys to %LOCALAPPDATA%/"
        "steam-tracer-cleaner/backups/<timestamp>/, then perform the delete. Steam is closed "
        "automatically. Restorable from the Backups tab.");
    ImGui::SameLine();
    if (ImGui::Button("Clean", ImVec2(120, 34))) {
        run_clean(state, false);
    }
    stc::ui::hover_tooltip(
        "Delete without a backup. Steam is closed automatically. Faster but irreversible.");

    ImGui::Spacing();

    if (state.last_plan) {
        const auto& plan = *state.last_plan;
        ImGui::Text("Plan: %zu steps, %s, %llu files affected", plan.steps.size(),
                    stc::ui::format_bytes(plan.total_bytes).c_str(),
                    static_cast<unsigned long long>(plan.total_file_count));

        ImGui::BeginChild("##planlist", ImVec2(0, 180), ImGuiChildFlags_Borders);
        for (const auto& step : plan.steps) {
            const char* kind = "";
            switch (step.op.kind) {
                case stc::core::OpKind::RemoveFile: kind = "FILE"; break;
                case stc::core::OpKind::RemoveTree: kind = "DIR"; break;
                case stc::core::OpKind::RemoveRegistryValue: kind = "REGV"; break;
                case stc::core::OpKind::RemoveRegistryKey: kind = "REGK"; break;
                case stc::core::OpKind::ClearRegistryValue: kind = "CLEAR"; break;
                case stc::core::OpKind::WriteRegistryString: kind = "WRITE"; break;
                case stc::core::OpKind::VdfRemoveChild: kind = "VDF"; break;
                case stc::core::OpKind::VdfSetValue: kind = "VDF-SET"; break;
                case stc::core::OpKind::ClearBrowserSteamCookies: kind = "COOKIES"; break;
            }
            std::string row = std::string{"["} + kind + "] " + stc::ui::to_utf8(step.op.target);
            if (!step.op.value_name.empty()) {
                row += " :: " + stc::ui::to_utf8(step.op.value_name);
            }
            if (step.op.kind == stc::core::OpKind::WriteRegistryString ||
                step.op.kind == stc::core::OpKind::VdfSetValue) {
                row += " = \"" + stc::ui::to_utf8(step.op.payload) + "\"";
            }
            ImGui::TextWrapped("%s", row.c_str());
        }
        ImGui::EndChild();
    }

    if (state.last_result) {
        ImGui::Spacing();
        ImGui::Text("Result: %d succeeded, %d failed, %s freed", state.last_result->succeeded,
                    state.last_result->failed,
                    stc::ui::format_bytes(state.last_result->bytes_freed).c_str());
        if (!state.last_result->failure_messages.empty() &&
            ImGui::CollapsingHeader("Failures")) {
            ImGui::Indent();
            for (const auto& m : state.last_result->failure_messages) {
                ImGui::TextWrapped("%s", stc::ui::to_utf8(m).c_str());
            }
            ImGui::Unindent();
        }
    }

}

}  // namespace stc::ui::screens
