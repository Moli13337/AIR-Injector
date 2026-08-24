#include "screen.h"

#include <string>
#include <windows.h>
#include <shellapi.h>

#include "process/process.h"
#include "folder/folder.h"
#include "update/update.h"
#include "base.h"
#include <iostream>

#include <thread>

#define WINDOW_WIDTH 700
#define WINDOW_HEIGHT 400

void Screen::SetupStyle()
{
	// Deep Dark style by janekb04 from ImThemes
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.6000000238418579f;
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.WindowRounding = 7.0f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(32.0f, 32.0f);
	style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Left;
	style.ChildRounding = 4.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 4.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(10.0f, 4.0f);
	style.FrameRounding = 3.0f;
	style.FrameBorderSize = 1.0f;
	style.ItemSpacing = ImVec2(6.0f, 6.0f);
	style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
	style.CellPadding = ImVec2(6.0f, 6.0f);
	style.IndentSpacing = 25.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 15.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 3.0f;
	style.TabRounding = 4.0f;
	style.TabBorderSize = 1.0f;
	style.TabMinWidthForCloseButton = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

	style.Colors[ImGuiCol_Text] = ImVec4(0.93f, 0.93f, 0.95f, 1.0f); // #EDEDF2
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.63f, 0.64f, 0.69f, 1.0f); // #A0A3B1
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.07f, 1.0f); // #0F0F12
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.06f, 0.07f, 0.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.92f); // #141419
	style.Colors[ImGuiCol_Border] = ImVec4(0.14f, 0.14f, 0.18f, 0.29f); // #24242E
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.24f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.13f, 0.54f); // #1A1A22
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.14f, 0.14f, 0.18f, 0.54f); // #24242E
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.20f, 1.0f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.07f, 1.0f); // #0F0F12
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.10f, 1.0f); // #141419
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.06f, 0.06f, 0.07f, 1.0f); // #0F0F12
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.13f, 1.0f); // #1A1A22
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.13f, 0.54f); // #1A1A22
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.14f, 0.14f, 0.18f, 0.54f); // #24242E
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.18f, 0.22f, 0.54f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.22f, 0.22f, 0.26f, 0.54f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.48f, 0.36f, 1.0f, 1.0f); // #7A5CFF
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.48f, 0.36f, 1.0f, 0.54f); // #7A5CFF
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.60f, 0.52f, 1.0f, 0.54f); // #9A84FF
	style.Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.10f, 0.13f, 0.54f); // #1A1A22
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.14f, 0.14f, 0.18f, 0.54f); // #24242E
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.16f, 0.20f, 1.0f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.06f, 0.06f, 0.07f, 0.52f); // #0F0F12
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.06f, 0.06f, 0.07f, 0.36f); // #0F0F12
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.16f, 0.16f, 0.20f, 0.33f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.14f, 0.14f, 0.18f, 0.29f); // #24242E
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.18f, 0.18f, 0.22f, 0.29f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.22f, 0.22f, 0.26f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.14f, 0.14f, 0.18f, 0.29f); // #24242E
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.18f, 0.18f, 0.22f, 0.29f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.22f, 0.22f, 0.26f, 1.0f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.06f, 0.06f, 0.07f, 0.52f); // #0F0F12
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.10f, 0.10f, 0.13f, 1.0f); // #1A1A22
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.16f, 0.16f, 0.20f, 0.36f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06f, 0.06f, 0.07f, 0.52f); // #0F0F12
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.10f, 0.10f, 0.13f, 1.0f); // #1A1A22
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.48f, 0.36f, 1.0f, 1.0f); // #7A5CFF
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.60f, 0.52f, 1.0f, 1.0f); // #9A84FF
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.48f, 0.36f, 1.0f, 1.0f); // #7A5CFF
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.60f, 0.52f, 1.0f, 1.0f); // #9A84FF
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.06f, 0.06f, 0.07f, 0.52f); // #0F0F12
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.06f, 0.06f, 0.07f, 0.52f); // #0F0F12
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.14f, 0.14f, 0.18f, 0.29f); // #24242E
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.06f, 0.06f, 0.07f, 0.0f); // #0F0F12
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.93f, 0.93f, 0.95f, 0.06f); // #EDEDF2
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.16f, 0.16f, 0.20f, 1.0f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.48f, 0.36f, 1.0f, 1.0f); // #7A5CFF
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.48f, 0.36f, 1.0f, 1.0f); // #7A5CFF
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.48f, 0.36f, 1.0f, 0.7f); // #7A5CFF
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.48f, 0.36f, 1.0f, 0.2f); // #7A5CFF
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.48f, 0.36f, 1.0f, 0.35f); // #7A5CFF
}


#include <iostream>

bool Screen::Render()
{
	static std::vector<ProcessManager::WindowInfo> processes;
	static int selectedProcess = 0;
	static bool isScanning = true;
	static float scanProgress = 0.0f;
	static bool minecraftFound = false;
	static int scanAttempts = 0;
	static const int MAX_SCAN_ATTEMPTS = 50; // ~2.5 seconds

	// Simulate scanning animation
	if (isScanning)
	{
		scanProgress += 0.02f;
		if (scanProgress >= 1.0f)
		{
			scanProgress = 0.0f;
			ProcessManager::GetMinecraftProcesses(processes);
			minecraftFound = processes.size() > 0;
			scanAttempts++;
			
			if (minecraftFound)
			{
				isScanning = false;
			}
			else if (scanAttempts >= MAX_SCAN_ATTEMPTS)
			{
				// Auto-close if no Minecraft found after max attempts
				if (m_hwnd) {
					PostMessage(m_hwnd, WM_CLOSE, 0, 0);
				}
				return false;
			}
		}
	}
	else
	{
		// Continue scanning periodically
		static int scanCounter = 0;
		scanCounter++;
		if (scanCounter % 100 == 0)
		{
			ProcessManager::GetMinecraftProcesses(processes);
			minecraftFound = processes.size() > 0;
			if (!minecraftFound)
			{
				isScanning = true;
				scanAttempts = 0;
			}
		}
	}

	if (selectedProcess >= processes.size())
	{
		selectedProcess = 0;
	}

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::Begin("AirV1 Injector", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
	ImGui::PopStyleVar();

	// Draw title at top
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("AirV1").x / 2);
	ImGui::SetCursorPosY(30);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.48f, 0.36f, 1.0f, 1.0f)); // #7A5CFF
	ImGui::Text("AirV1");
	ImGui::PopStyleColor();

	// Discord button - top right
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 100);
	ImGui::SetCursorPosY(10);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.48f, 0.36f, 1.0f, 1.0f)); // #7A5CFF
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.60f, 0.52f, 1.0f, 1.0f)); // #9A84FF
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.25f, 0.88f, 1.0f)); // #5A3FE0
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5));
	if (ImGui::Button("Discord"))
	{
		ShellExecuteW(0, L"open", L"https://discord.gg/dDN4BMQfBA", 0, 0, SW_SHOWNORMAL);
	}
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(3);

	// Draw scanning progress bar
	if (isScanning)
	{
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 200);
		ImGui::SetCursorPosY(80);
		
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.13f, 1.0f)); // #1A1A22
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.48f, 0.36f, 1.0f, 1.0f)); // #7A5CFF
		ImGui::ProgressBar(scanProgress, ImVec2(400, 10), "");
		ImGui::PopStyleColor(2);
		
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("Scanning for Minecraft...").x / 2);
		ImGui::SetCursorPosY(100);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.63f, 0.64f, 0.69f, 1.0f)); // #A0A3B1
		ImGui::Text("Scanning for Minecraft...");
		ImGui::PopStyleColor();
	}
	else if (minecraftFound)
	{
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("Minecraft Found!").x / 2);
		ImGui::SetCursorPosY(80);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.48f, 0.36f, 1.0f, 1.0f)); // #7A5CFF
		ImGui::Text("Minecraft Found!");
		ImGui::PopStyleColor();

		// Inject button - only show when Minecraft is found
		std::string text = "Inject | " + (processes.size() > 0 ? processes[selectedProcess].processName : "NONE");
		float stringWidth = ImGui::CalcTextSize(text.c_str()).x + 40;
		float stringHeight = ImGui::CalcTextSize(text.c_str()).y + 40;
		ImGui::SetCursorPosY((ImGui::GetWindowHeight() / 2) - (stringHeight / 2) + 20);
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() / 2) - (stringWidth / 2));

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.48f, 0.36f, 1.0f, 1.0f)); // #7A5CFF
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.60f, 0.52f, 1.0f, 1.0f)); // #9A84FF
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.25f, 0.88f, 1.0f)); // #5A3FE0
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 20));
		if (ImGui::Button(text.c_str()))
		{
			bool minecraft_running = processes.size() > 0;
			if (minecraft_running)
			{
				if (ProcessManager::InjectDLL(processes[selectedProcess].processId, FolderManager::GetDllPath().c_str()))
				{
					// Show "Injected" and close
					static bool injectedShown = false;
					injectedShown = true;
					Sleep(1000); // Show for 1 second
					if (m_hwnd) {
						PostMessage(m_hwnd, WM_CLOSE, 0, 0);
					}
				}
				else
				{
					MessageBoxA(NULL, "Failed to inject!", "AirV1 Injector", MB_OK | MB_ICONERROR);
				}
			}
			else
			{
				MessageBoxA(NULL, "Minecraft is not running!", "AirV1 Injector", MB_OK | MB_ICONERROR);
			}
		}
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);

		bool minecraft_running = processes.size() > 0;
		bool multiple_minecraft_instances = processes.size() > 1;

		if (multiple_minecraft_instances)
		{
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 93);
			ImGui::SetCursorPosY((ImGui::GetWindowHeight() / 2) - (stringHeight / 2) + 80);
			if (ImGui::Button("Switch Minecraft Instance"))
			{
				selectedProcess++;
				if (selectedProcess >= processes.size())
				{
					selectedProcess = 0;
				}
			}
		}
	}

	ImGui::End();

	return true;
}