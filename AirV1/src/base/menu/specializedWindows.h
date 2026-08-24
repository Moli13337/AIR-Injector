#pragma once

#include <string>
#include <functional>
#include <imgui.h>

namespace SpecializedWindows {
    // Render a single specialized window at top center
    // with minimal styling - text with drop shadow and underline
    void RenderSingleWindow(
        const std::string& windowName,
        const std::string& title,
        std::function<void()> renderCallback,
        bool& isOpen,
        bool isSelfDestruct = false
    );

    // Draw text with drop shadow effect
    void DrawTextWithShadow(const char* text, ImVec2 pos, ImU32 color, ImU32 shadowColor = IM_COL32(0, 0, 0, 150));

    // Draw underline for title
    void DrawUnderline(ImVec2 start, ImVec2 end, ImU32 color, float thickness = 2.0f);
}
