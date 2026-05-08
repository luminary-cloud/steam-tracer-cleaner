#pragma once

#include <set>
#include <string>

#include "core/targets.hpp"

namespace stc::ui::widgets {

// Renders a checkbox tree of every target in the catalog, grouped by category. Mutates `selected`
// in place. Returns true if any selection changed this frame.
bool target_tree(std::set<std::string>& selected);

}  // namespace stc::ui::widgets
