#pragma once

#include "moduleManager/moduleBase.h"

class AutoTool : public ModuleBase
{
public:
	void Update() override;

	void RenderOverlay() override {};
	void RenderHud() override {};
	void RenderMenu() override;

	std::string GetName() override { return m_name; }
	std::string GetCategory() override { return m_category; }
	int GetKey() override { return settings::AT_Key; }

	bool IsEnabled() override { return settings::AT_Enabled; }
	void SetEnabled(bool enabled) override { settings::AT_Enabled = enabled; }
	void Toggle() override { settings::AT_Enabled = !settings::AT_Enabled; }

private:
	std::string m_name = "Auto Tool";
	std::string m_category = "Inventory";
};
