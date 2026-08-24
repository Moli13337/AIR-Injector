#include "arrayList.h"

#include "util/render/render.h"
#include "menu/menu.h"
#include "moduleManager/commonData.h"
#include "moduleManager/moduleManager.h"

void ArrayList::RenderHud() {
    if (!settings::AL_Enabled) return;

    std::vector<std::string> enabledModules;
    
    // Collect enabled modules
	for (std::unique_ptr<ModuleBase>& module : ModuleManager::GetModules())
    {
        if (module->IsEnabled())
        {
            enabledModules.push_back(module->GetName());
        }
	}

    if (enabledModules.empty()) return;

    // Sort modules by length
    std::sort(enabledModules.begin(), enabledModules.end(), 
        [](const std::string& a, const std::string& b) { 
            return Menu::font->CalcTextSizeA(settings::AL_textSize, FLT_MAX, 0.0f, a.c_str()).x > 
                   Menu::font->CalcTextSizeA(settings::AL_textSize, FLT_MAX, 0.0f, b.c_str()).x; 
        });

    ImVec4 bgColor(0.0f, 0.0f, 0.0f, 0.0f);

    Render::RenderModuleList(
        enabledModules,
        settings::AL_renderPosition,
        settings::AL_textSize,
        0.0f,
        bgColor,
        Menu::font,
        settings::AL_colorMode != 0,
        settings::AL_colorMode,
        settings::AL_rgbSpeed
    );
}

void ArrayList::RenderMenu()
{
    Menu::ToggleWithKeybind(&settings::AL_Enabled, settings::AL_Key);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
    Menu::HorizontalSeparator("Sep1");
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);

	Menu::Dropdown("Position", settings::AL_renderPositionList, &settings::AL_renderPosition, 4);
	Menu::Slider("Text Size", &settings::AL_textSize, 1, 50);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
    Menu::HorizontalSeparator("Sep2");
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);

	Menu::Dropdown("Color Mode", settings::AL_colorModeList, &settings::AL_colorMode, 15);
	if (settings::AL_colorMode == 0)
	{
		Menu::ColorEdit("Color", settings::AL_textColor);
	}
    else
    {
		Menu::Slider("Chroma Speed", &settings::AL_rgbSpeed, 0.1f, 5.0f, ImVec2(0,0), "%.1f");
    }
}
