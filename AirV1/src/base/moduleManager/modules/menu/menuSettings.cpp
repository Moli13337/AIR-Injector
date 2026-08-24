#include "menuSettings.h"
#include "menu/menu.h"
#include "sdk/sdk.h"
#include "configManager/settings.h"

void MenuSettings::RenderMenu()
{
	Menu::Slider("Theme Hue", &settings::Menu_ThemeHue, 0.0f, 1.0f);
	
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
	Menu::Text("Theme Color Preview", FontSize::SIZE_16);
	
	// Draw color preview
	ImVec4 themeColor;
	ImGui::ColorConvertHSVtoRGB(settings::Menu_ThemeHue, 0.75f, 0.6f, themeColor.x, themeColor.y, themeColor.z);
	ImGui::ColorButton("##ThemePreview", themeColor, 0, ImVec2(50, 20));
}
