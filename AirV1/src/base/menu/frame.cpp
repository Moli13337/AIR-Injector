#include "frame.h"
#include "menu/menu.h"
#include "sdk/sdk.h"
#include "configManager/settings.h"
#include <gl/GL.h>

// Static member initialization
std::unordered_map<std::string, bool> Frame::moduleExpanded;

Frame::Frame(std::string name, int index) : name(name), index(index) {
    // Initialize position in horizontal row
    x = 10.0f + (index * 110.0f);
    y = 10.0f;
}

void Frame::addModule(ModuleBase* module) {
    modules.push_back(module);
}

void Frame::draw(int mouseX, int mouseY) {
    // Calculate total height based on modules and expanded settings
    float currentY = headerHeight;
    height = headerHeight;
    
    for (auto* module : modules) {
        // Module button height
        float moduleHeight = 20.0f;
        
        // Check if settings are expanded
        bool isExpanded = moduleExpanded[module->GetName()];
        float settingsHeight = isExpanded ? 80.0f : 0.0f; // Settings area height
        
        height += moduleHeight + settingsHeight;
        currentY += moduleHeight + settingsHeight;
    }
    
    // Draw frame background (solid black with theme accent)
    ImVec4 themeColor;
    ImGui::ColorConvertHSVtoRGB(settings::Menu_ThemeHue, 0.75f, 0.6f, themeColor.x, themeColor.y, themeColor.z);
    
    // Draw frame border with theme color
    ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(themeColor);
    ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 0.95f));
    
    ImGui::GetWindowDrawList()->AddRect(
        ImVec2(x, y),
        ImVec2(x + width, y + height),
        borderColor,
        4.0f
    );
    
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(x, y),
        ImVec2(x + width, y + height),
        bgColor,
        4.0f
    );
    
    // Draw header
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(x, y),
        ImVec2(x + width, y + headerHeight),
        borderColor,
        4.0f,
        ImDrawFlags_RoundCornersTop
    );
    
    // Draw category name
    ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(x + (width - textSize.x) / 2, y + (headerHeight - textSize.y) / 2),
        ImColor(1.0f, 1.0f, 1.0f, 1.0f),
        name.c_str()
    );
    
    // Draw modules with dynamic vertical stacking
    currentY = y + headerHeight;
    for (auto* module : modules) {
        float moduleHeight = 20.0f;
        bool isExpanded = moduleExpanded[module->GetName()];
        float settingsHeight = isExpanded ? 80.0f : 0.0f;
        
        // Module background
        ImU32 moduleBgColor = module->IsEnabled() ? 
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.1f, 0.1f, 0.1f, 0.8f)) :
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.05f, 0.05f, 0.05f, 0.8f));
        
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(x + 2, currentY),
            ImVec2(x + width - 2, currentY + moduleHeight),
            moduleBgColor,
            2.0f
        );
        
        // Module name (centered, no arrows)
        ImVec2 modTextSize = ImGui::CalcTextSize(module->GetName().c_str());
        ImColor textColor = module->IsEnabled() ? 
            ImColor(1.0f, 1.0f, 1.0f, 1.0f) : 
            ImColor(0.6f, 0.6f, 0.6f, 1.0f);
        
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(x + (width - modTextSize.x) / 2, currentY + (moduleHeight - modTextSize.y) / 2),
            textColor,
            module->GetName().c_str()
        );
        
        // Draw expanded settings below module
        if (isExpanded) {
            float settingsY = currentY + moduleHeight;
            ImU32 settingsBgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.08f, 0.08f, 0.08f, 0.9f));
            
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(x + 2, settingsY),
                ImVec2(x + width - 2, settingsY + settingsHeight),
                settingsBgColor,
                2.0f
            );
            
            // Render module settings
            ImGui::SetCursorScreenPos(ImVec2(x + 5, settingsY + 5));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2));
            
            // Clip to settings area
            ImGui::PushClipRect(
                ImVec2(x + 2, settingsY),
                ImVec2(x + width - 2, settingsY + settingsHeight),
                true
            );
            
            module->RenderMenu();
            
            ImGui::PopClipRect();
            ImGui::PopStyleVar(2);
            
            // Draw inline HSB color picker for color settings
            // This is a simplified version - in a full implementation, you'd detect color settings
            // and render the picker accordingly
            static bool showColorPicker = false;
            if (showColorPicker) {
                float pickerSize = 40.0f;
                float pickerX = x + (width - pickerSize) / 2;
                float pickerY = settingsY + settingsHeight - pickerSize - 5;
                
                // Draw HSB square
                for (int py = 0; py < (int)pickerSize; py++) {
                    for (int px = 0; px < (int)pickerSize; px++) {
                        float hue = px / pickerSize;
                        float saturation = 1.0f - (py / pickerSize);
                        ImVec4 hsvColor = ImVec4(hue, saturation, 1.0f, 1.0f);
                        ImVec4 rgbColor;
                        ImGui::ColorConvertHSVtoRGB(hsvColor.x, hsvColor.y, hsvColor.z, rgbColor.x, rgbColor.y, rgbColor.z);
                        ImU32 color = ImGui::ColorConvertFloat4ToU32(rgbColor);
                        ImGui::GetWindowDrawList()->AddLine(
                            ImVec2(pickerX + px, pickerY + py),
                            ImVec2(pickerX + px + 1, pickerY + py),
                            color
                        );
                    }
                }
            }
        }
        
        currentY += moduleHeight + settingsHeight;
    }
}

void Frame::mouseClicked(int mouseX, int mouseY, int button) {
    // Check if click is in header
    if (mouseX >= x && mouseX <= x + width && mouseY >= y && mouseY <= y + headerHeight) {
        if (button == 0) { // Left click
            isDragging = true;
            dragOffsetX = mouseX - x;
            dragOffsetY = mouseY - y;
        }
    }
    
    // Check if click is in modules
    float currentY = y + headerHeight;
    for (auto* module : modules) {
        float moduleHeight = 20.0f;
        bool isExpanded = moduleExpanded[module->GetName()];
        float settingsHeight = isExpanded ? 80.0f : 0.0f;
        float totalHeight = moduleHeight + settingsHeight;
        
        if (mouseX >= x && mouseX <= x + width && mouseY >= currentY && mouseY <= currentY + moduleHeight) {
            if (button == 0) { // Left click - toggle module
                module->Toggle();
            } else if (button == 1) { // Right click - toggle settings expansion
                moduleExpanded[module->GetName()] = !moduleExpanded[module->GetName()];
            }
            break;
        }
        
        currentY += totalHeight;
    }
}

void Frame::mouseReleased(int mouseX, int mouseY, int button) {
    if (button == 0) {
        isDragging = false;
    }
}

void Frame::mouseDragged(int mouseX, int mouseY) {
    if (isDragging) {
        x = mouseX - dragOffsetX;
        y = mouseY - dragOffsetY;
    }
}
