#include "configSettings.h"
#include "menu/menu.h"
#include "sdk/sdk.h"
#include "configManager/configManager.h"

void ConfigSettings::RenderMenu()
{
	static int selectedConfig = -1;
	static std::vector<std::string> configList = configmanager::GetConfigList();
	
	// Config list dropdown
	if (!configList.empty())
	{
		std::vector<const char*> configNames;
		for (const auto& config : configList)
		{
			configNames.push_back(config.c_str());
		}
		
		if (selectedConfig < 0 || selectedConfig >= configList.size())
		{
			selectedConfig = 0;
		}
		
		Menu::Dropdown("Config", configNames.data(), &selectedConfig, configList.size());
		
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
		
		// Config buttons
		if (Menu::Button("Load"))
		{
			if (selectedConfig >= 0 && selectedConfig < configList.size())
			{
				configmanager::LoadConfig(selectedConfig);
			}
		}
		
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
		
		if (Menu::Button("Save"))
		{
			if (selectedConfig >= 0 && selectedConfig < configList.size())
			{
				configmanager::SaveConfig(configList[selectedConfig].c_str());
			}
		}
	}
	else
	{
		Menu::Text("No configs found", FontSize::SIZE_16);
	}
}
