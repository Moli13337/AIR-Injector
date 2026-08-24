#pragma once

#include "moduleManager/moduleBase.h"
#include "configManager/settings.h"

class ConfigSettings : public ModuleBase
{
public:
	void Update() override {};

	void RenderOverlay() override {};
	void RenderHud() override {};
	void RenderMenu() override;

	std::string GetName() override { return m_name; }
	std::string GetCategory() override { return m_category; }
	int GetKey() override { return 0; }

	bool IsEnabled() override { return false; }
	void SetEnabled(bool enabled) override {}
	void Toggle() override {}

private:
	std::string m_name = "Config";
	std::string m_category = "Config";
};
