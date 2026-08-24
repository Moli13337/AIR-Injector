#include "specializedWindows.h"
#include "menu/menu.h"
#include <imgui.h>

namespace SpecializedWindows {
    void DrawTextWithShadow(const char* text, ImVec2 pos, ImU32 color, ImU32 shadowColor) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        // Draw shadow (offset by 1px)
        drawList->AddText(ImVec2(pos.x + 1, pos.y + 1), shadowColor, text);
        
        // Draw main text
        drawList->AddText(pos, color, text);
    }

    void DrawUnderline(ImVec2 start, ImVec2 end, ImU32 color, float thickness) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddLine(start, end, color, thickness);
    }

    void RenderSingleWindow(
        const std::string& windowName,
        const std::string& title,
        std::function<void()> renderCallback,
        bool& isOpen,
        bool isSelfDestruct
    ) {
        if (!isOpen) return;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 viewportSize = viewport->Size;

        // Position at top center (x = ScreenWidth/2, y = 20)
        float windowWidth = 400.0f;
        float windowX = (viewportSize.x - windowWidth) * 0.5f;
        float windowY = 20.0f;

        ImGui::SetNextWindowPos(ImVec2(windowX, windowY));
        ImGui::SetNextWindowSize(ImVec2(windowWidth, 50.0f)); // Compact height for title
        
        // No background, no decorations for title bar
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        
        ImGui::Begin(windowName.c_str(), &isOpen, 
            ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoTitleBar | 
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoScrollbar);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 windowPos = ImGui::GetWindowPos();

        // Title color (red for SelfDestruct, purple for others)
        ImU32 titleColor = isSelfDestruct ? 
            ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.2f, 0.2f, 1.0f)) :
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.48f, 0.36f, 1.0f, 1.0f)); // #7A5CFF

        // Draw title text with shadow
        ImVec2 textSize = ImGui::CalcTextSize(title.c_str());
        ImVec2 textPos = ImVec2(
            windowPos.x + (windowWidth - textSize.x) * 0.5f,
            windowPos.y + 15
        );
        DrawTextWithShadow(title.c_str(), textPos, titleColor);

        // Draw underline
        DrawUnderline(
            ImVec2(windowPos.x + 10, windowPos.y + 38),
            ImVec2(windowPos.x + windowWidth - 10, windowPos.y + 38),
            titleColor,
            2.0f
        );

        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        // Render content window below title
        float contentY = windowY + 50.0f;
        ImGui::SetNextWindowPos(ImVec2(windowX, contentY));
        ImGui::SetNextWindowSize(ImVec2(windowWidth, 300.0f));
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.07f, 0.9f)); // #0F0F12
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
        
        std::string contentWindowName = windowName + "_Content";
        ImGui::Begin(contentWindowName.c_str(), nullptr,
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoTitleBar);
        
        if (renderCallback) {
            renderCallback();
        }
        
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }
}
