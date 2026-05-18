#pragma once

#include <imgui.h>

struct ID3D11Device;

namespace stc::ui::icons {

void load(ID3D11Device* device);
void shutdown();

ImTextureID github();   // null if decode failed; callers should provide a text fallback

}  // namespace stc::ui::icons
