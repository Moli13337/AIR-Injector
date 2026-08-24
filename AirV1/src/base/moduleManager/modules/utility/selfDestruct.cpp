#include "selfDestruct.h"

void SelfDestruct::RenderMenu()
{
	static int selectedAction = 0;
	static const char* actions[] = {
		"Detach",
		"Clean Logs",
		"Delete DLL",
		"Delete EXE",
		"Full SelfDestruct"
	};

	auto closeAndDetach = []()
	{
		Menu::open = false;
		Sleep(100);
		Base::m_running = false;
	};

	auto cleanLogs = []()
	{
		char appdata[MAX_PATH];
		if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata) != S_OK)
			return;

		std::string logsPath = std::string(appdata) + "\\.minecraft\\logs";
		try
		{
			for (const auto& entry : std::filesystem::directory_iterator(logsPath))
			{
				if (entry.path().extension() == ".log" || entry.path().extension() == ".gz")
				{
					std::filesystem::remove(entry.path());
				}
			}
			MessageBoxA(NULL, "Logs cleaned successfully!", "AirV1", MB_OK | MB_ICONINFORMATION);
		}
		catch (...)
		{
			MessageBoxA(NULL, "Failed to clean logs.", "AirV1", MB_OK | MB_ICONERROR);
		}
	};

	auto deleteDll = []()
	{
		Menu::open = false;
		Sleep(100);

		char dllPath[MAX_PATH];
		GetModuleFileNameA(GetModuleHandleA("AirV1.dll"), dllPath, MAX_PATH);

		std::string batchCmd = "ping 127.0.0.1 -n 2 > nul & del \"" + std::string(dllPath) + "\"";
		std::string batchPath = std::string(dllPath).substr(0, std::string(dllPath).find_last_of("\\\\")) + "\\delete_dll.bat";
		std::ofstream batchFile(batchPath);
		batchFile << batchCmd;
		batchFile.close();

		ShellExecuteA(NULL, "open", batchPath.c_str(), NULL, NULL, SW_HIDE);
		Base::m_running = false;
	};

	auto deleteExe = []()
	{
		Menu::open = false;
		Sleep(100);

		char exePath[MAX_PATH];
		GetModuleFileNameA(NULL, exePath, MAX_PATH);
		try
		{
			std::string batchCmd = "ping 127.0.0.1 -n 2 > nul & del \"" + std::string(exePath) + "\"";
			std::string batchPath = std::string(exePath).substr(0, std::string(exePath).find_last_of("\\\\")) + "\\delete_exe.bat";
			std::ofstream batchFile(batchPath);
			batchFile << batchCmd;
			batchFile.close();
			ShellExecuteA(NULL, "open", batchPath.c_str(), NULL, NULL, SW_HIDE);
			Base::m_running = false;
		}
		catch (...)
		{
			MessageBoxA(NULL, "Failed to delete EXE.", "AirV1", MB_OK | MB_ICONERROR);
		}
	};

	auto fullSelfDestruct = [&]()
	{
		MessageBoxA(NULL, "Full SelfDestruct initiated...", "AirV1", MB_OK | MB_ICONWARNING);

		char appdata[MAX_PATH];
		if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata) == S_OK)
		{
			std::string logsPath = std::string(appdata) + "\\.minecraft\\logs";
			try
			{
				for (const auto& entry : std::filesystem::directory_iterator(logsPath))
				{
					if (entry.path().extension() == ".log" || entry.path().extension() == ".gz")
					{
						std::filesystem::remove(entry.path());
					}
				}
			}
			catch (...) {}
		}

		Menu::open = false;
		Sleep(100);

		char dllPath[MAX_PATH];
		char exePath[MAX_PATH];
		GetModuleFileNameA(GetModuleHandleA("AirV1.dll"), dllPath, MAX_PATH);
		GetModuleFileNameA(NULL, exePath, MAX_PATH);

		std::string batchCmd = "ping 127.0.0.1 -n 2 > nul & del \"" + std::string(dllPath) + "\" & del \"" + std::string(exePath) + "\"";
		std::string batchPath = std::string(exePath).substr(0, std::string(exePath).find_last_of("\\\\")) + "\\full_selfdestruct.bat";
		std::ofstream batchFile(batchPath);
		batchFile << batchCmd;
		batchFile.close();

		ShellExecuteA(NULL, "open", batchPath.c_str(), NULL, NULL, SW_HIDE);
		Base::m_running = false;
	};

	Menu::Text("Action", FontSize::SIZE_18);
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	ImGui::Combo("##self_destruct_action", &selectedAction, actions, IM_ARRAYSIZE(actions));

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12.0f);
	const bool destructive = selectedAction == 4;
	if (destructive)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
	}

	if (Menu::Button("EXECUTE", ImVec2(ImGui::GetContentRegionAvail().x, 30.0f), FontSize::SIZE_16))
	{
		switch (selectedAction)
		{
		case 0:
			closeAndDetach();
			break;
		case 1:
			cleanLogs();
			break;
		case 2:
			deleteDll();
			break;
		case 3:
			deleteExe();
			break;
		case 4:
			fullSelfDestruct();
			break;
		default:
			break;
		}
	}

	if (destructive)
		ImGui::PopStyleColor(3);
}
