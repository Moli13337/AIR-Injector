#include "clientBrandChanger.h"
#include "menu/menu.h"
#include "base/base.h"
#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>

void ClientBrandChanger::RenderMenu()
{
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.48f, 0.36f, 1.0f, 1.0f)); // #7A5CFF
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.60f, 0.52f, 1.0f, 1.0f)); // #9A84FF
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.25f, 0.88f, 1.0f)); // #5A3FE0
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 8));
	
	Menu::Checkbox("Enabled", &settings::CBC_Enabled);
	
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
	Menu::HorizontalSeparator("Sep1");
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
	
	static char clientBrand[128] = "";
	if (settings::CBC_ClientBrand.empty()) {
		strcpy_s(clientBrand, sizeof(clientBrand), "AirV1");
	} else {
		strcpy_s(clientBrand, sizeof(clientBrand), settings::CBC_ClientBrand.c_str());
	}
	
	ImGui::SetNextItemWidth(ImGui::GetWindowSize().x - 30.f);
	ImGui::InputText("##clientBrand", clientBrand, IM_ARRAYSIZE(clientBrand));
	
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
	
	if (ImGui::Button("Set Brand", ImVec2(ImGui::GetWindowSize().x - 30.f, 30.f))) {
		settings::CBC_ClientBrand = std::string(clientBrand);
		std::call_once(m_setOriginalClientBrandFlag, []() {
			// Store original brand on first set
		});
	}
	
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(3);
}

void ClientBrandChanger::OnGetClientModName(JNIEnv* env, bool* cancel)
{
	if (!env || !cancel || !settings::CBC_Enabled || settings::CBC_ClientBrand.empty()) {
		return;
	}

	jstring brand = env->NewStringUTF(settings::CBC_ClientBrand.c_str());
	if (brand)
	{
		JavaHook::SetReturnValue<void*>(cancel, *(void**)brand);
		*cancel = true;
	}
}

void ClientBrandChanger::GetClientModName_callback(HotSpot::frame* frame, HotSpot::Thread* thread, bool* cancel)
{
	if (!thread || !cancel)
		return;

	JNIEnv* env = thread->GetEnv();
	OnGetClientModName(env, cancel);
}
