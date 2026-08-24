#pragma once

#include <string>

#include "moduleManager/moduleBase.h"
#include "menu/menu.h"
#include "base/base.h"
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <filesystem>
#include <fstream>

class SelfDestruct : public ModuleBase
{
public:
	void Update() override {}

	void RenderOverlay() override {};
	void RenderHud() override {};
	void RenderMenu() override;

	std::string GetName() override { return m_name; }
	std::string GetCategory() override { return m_category; }
	int GetKey() override { return settings::Menu_DetachKey; }

	bool IsEnabled() override { return false; } // Always disabled, just a utility module
	void SetEnabled(bool enabled) override {} // Not used
	void Toggle() override {} // Not used

private:
	std::string m_name = "SelfDestruct";
	std::string m_category = "Utility";
};
