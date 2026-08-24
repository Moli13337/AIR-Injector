#include "menu.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <imgui/imgui_internal.h>
#include <gl/GL.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#include "util/logger.h"
#include "util/keys.h"
#include "util/string.h"
#include "configManager/settings.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif

namespace
{
	ULONG_PTR g_gdiplusToken = 0;

	std::wstring WidenPath(const char* filename)
	{
		const int required = MultiByteToWideChar(CP_UTF8, 0, filename, -1, nullptr, 0);
		if (required <= 0)
			return L"";

		std::wstring wide(required - 1, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, filename, -1, wide.data(), required);
		return wide;
	}

	bool EnsureGdiplus()
	{
		if (g_gdiplusToken != 0)
			return true;

		Gdiplus::GdiplusStartupInput input;
		return Gdiplus::GdiplusStartup(&g_gdiplusToken, &input, nullptr) == Gdiplus::Ok;
	}

	void DeleteTexture(ImTextureID& texture)
	{
		if (!texture)
			return;

		const GLuint id = static_cast<GLuint>(reinterpret_cast<intptr_t>(texture));
		glDeleteTextures(1, &id);
		texture = nullptr;
	}

	ImU32 SliderAccent(float alpha = 1.0f, float value = 0.90f)
	{
		ImVec4 color;
		ImGui::ColorConvertHSVtoRGB(settings::Menu_ThemeHue, 0.74f, value, color.x, color.y, color.z);
		color.w = alpha;
		return ImGui::ColorConvertFloat4ToU32(color);
	}

	std::string PrettyKeyNameLocal(int key)
	{
		std::string name = Keys::GetKeyName(key);
		name.erase(std::remove(name.begin(), name.end(), '['), name.end());
		name.erase(std::remove(name.begin(), name.end(), ']'), name.end());

		if (name == "none") return "Unbound";
		if (name == "RSHIFT") return "Right Shift";
		if (name == "LSHIFT") return "Left Shift";
		if (name == "LCONTROL") return "Left Ctrl";
		if (name == "RCONTROL") return "Right Ctrl";
		if (name == "LMENU") return "Left Alt";
		if (name == "RMENU") return "Right Alt";
		return name;
	}
}

void Menu::Init()
{
	Menu::title = "AirV1";
	Menu::initialized = false;
	Menu::open = false;
	Menu::openHudEditor = false;

	Menu::PlaceHooks();
	LOG_INFO("Menu initialized");
}

void Menu::LoadIconTextures()
{
	const std::filesystem::path assetRoot = "C:\\Users\\Profile1\\Downloads\\AirV1\\AIRCLIENT\\assets";
	combatIconTexture = LoadTextureFromFile((assetRoot / "combat.png").string().c_str());
	movementIconTexture = LoadTextureFromFile((assetRoot / "movement.png").string().c_str());
	visualIconTexture = LoadTextureFromFile((assetRoot / "render.png").string().c_str());
	utilityIconTexture = LoadTextureFromFile((assetRoot / "utility.png").string().c_str());
	friendsIconTexture = LoadTextureFromFile("C:\\Users\\Profile1\\Downloads\\friends.png");
}

void Menu::CleanupIconTextures()
{
	DeleteTexture(combatIconTexture);
	DeleteTexture(movementIconTexture);
	DeleteTexture(visualIconTexture);
	DeleteTexture(utilityIconTexture);
	DeleteTexture(friendsIconTexture);
}

ImTextureID Menu::LoadTextureFromFile(const char* filename)
{
	if (!EnsureGdiplus())
	{
		LOG_WARNING("Failed to initialize GDI+ for icon texture: %s", filename);
		return nullptr;
	}

	const std::wstring widePath = WidenPath(filename);
	if (widePath.empty())
		return nullptr;

	Gdiplus::Bitmap bitmap(widePath.c_str());
	if (bitmap.GetLastStatus() != Gdiplus::Ok)
	{
		LOG_WARNING("Failed to load icon texture: %s", filename);
		return nullptr;
	}

	const UINT width = bitmap.GetWidth();
	const UINT height = bitmap.GetHeight();
	Gdiplus::Rect rect(0, 0, width, height);
	Gdiplus::BitmapData data{};
	if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) != Gdiplus::Ok)
	{
		LOG_WARNING("Failed to lock icon texture: %s", filename);
		return nullptr;
	}

	GLuint textureId = 0;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, data.Scan0);
	glBindTexture(GL_TEXTURE_2D, 0);

	bitmap.UnlockBits(&data);
	LOG_INFO("Loaded icon texture: %s", filename);
	return reinterpret_cast<ImTextureID>(static_cast<intptr_t>(textureId));
}

bool Menu::ConfigItem(const char* name, bool* deleted, bool scrollbar)
{
	ImVec2 size = ImGui::GetWindowSize();
	ImVec2 deleteBtnSize = Menu::font18->CalcTextSizeA(18, FLT_MAX, 0.0f, "Delete");
	deleteBtnSize.x += ImGui::GetStyle().FramePadding.x * 8;

	bool selected = ImGui::Button(name, ImVec2(size.x - deleteBtnSize.x - 18.f - (scrollbar ? ImGui::GetStyle().ScrollbarSize : 0.f), 0));

	ImGui::SameLine();

	if (ImGui::Button(("Delete###" + std::string(name)).c_str(), ImVec2(deleteBtnSize.x, 0.f)))
	{
		*deleted = true;
	}

	return selected;
}

void Menu::Text(const char* text, FontSize size)
{
	if (size == FontSize::SIZE_28) ImGui::PushFont(Menu::font28);
	else if (size == FontSize::SIZE_26) ImGui::PushFont(Menu::font26);
	else if (size == FontSize::SIZE_24) ImGui::PushFont(Menu::font24);
	else if (size == FontSize::SIZE_22) ImGui::PushFont(Menu::font22);
	else if (size == FontSize::SIZE_20) ImGui::PushFont(Menu::font20);
	else if (size == FontSize::SIZE_18) ImGui::PushFont(Menu::font18);
	else if (size == FontSize::SIZE_16) ImGui::PushFont(Menu::font16);
	else if (size == FontSize::SIZE_14) ImGui::PushFont(Menu::font14);

	ImGui::Text(text);

	ImGui::PopFont();
}

void Menu::BoldText(const char* text, FontSize size)
{
	if (size == FontSize::SIZE_28) ImGui::PushFont(Menu::boldFont28);
	else if (size == FontSize::SIZE_26) ImGui::PushFont(Menu::boldFont26);
	else if (size == FontSize::SIZE_24) ImGui::PushFont(Menu::boldFont24);
	else if (size == FontSize::SIZE_22) ImGui::PushFont(Menu::boldFont22);
	else if (size == FontSize::SIZE_20) ImGui::PushFont(Menu::boldFont20);
	else if (size == FontSize::SIZE_18) ImGui::PushFont(Menu::boldFont18);
	else if (size == FontSize::SIZE_16) ImGui::PushFont(Menu::boldFont16);
	else if (size == FontSize::SIZE_14) ImGui::PushFont(Menu::boldFont14);

	ImGui::Text(text);

	ImGui::PopFont();
}

void Menu::TextColored(const char* text, ImVec4 color, FontSize size)
{
	if (size == FontSize::SIZE_28) ImGui::PushFont(Menu::font28);
	else if (size == FontSize::SIZE_26) ImGui::PushFont(Menu::font26);
	else if (size == FontSize::SIZE_24) ImGui::PushFont(Menu::font24);
	else if (size == FontSize::SIZE_22) ImGui::PushFont(Menu::font22);
	else if (size == FontSize::SIZE_20) ImGui::PushFont(Menu::font20);
	else if (size == FontSize::SIZE_18) ImGui::PushFont(Menu::font18);
	else if (size == FontSize::SIZE_16) ImGui::PushFont(Menu::font16);
	else if (size == FontSize::SIZE_14) ImGui::PushFont(Menu::font14);

	ImGui::TextColored(color, text);

	ImGui::PopFont();
}

void Menu::BoldTextColored(const char* text, ImVec4 color, FontSize size)
{
	if (size == FontSize::SIZE_28) ImGui::PushFont(Menu::boldFont28);
	else if (size == FontSize::SIZE_26) ImGui::PushFont(Menu::boldFont26);
	else if (size == FontSize::SIZE_24) ImGui::PushFont(Menu::boldFont24);
	else if (size == FontSize::SIZE_22) ImGui::PushFont(Menu::boldFont22);
	else if (size == FontSize::SIZE_20) ImGui::PushFont(Menu::boldFont20);
	else if (size == FontSize::SIZE_18) ImGui::PushFont(Menu::boldFont18);
	else if (size == FontSize::SIZE_16) ImGui::PushFont(Menu::boldFont16);
	else if (size == FontSize::SIZE_14) ImGui::PushFont(Menu::boldFont14);

	ImGui::TextColored(color, text);

	ImGui::PopFont();
}

void Menu::GlitchText(const char* text, FontSize size)
{
	ImVec2 cursorPos = ImGui::GetCursorPos();  // Get the current cursor position in screen space
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// Push the custom font with the given size
	if (size == FontSize::SIZE_28) ImGui::PushFont(Menu::boldFont28);
	else if (size == FontSize::SIZE_26) ImGui::PushFont(Menu::boldFont26);
	else if (size == FontSize::SIZE_24) ImGui::PushFont(Menu::boldFont24);
	else if (size == FontSize::SIZE_22) ImGui::PushFont(Menu::boldFont22);
	else if (size == FontSize::SIZE_20) ImGui::PushFont(Menu::boldFont20);
	else if (size == FontSize::SIZE_18) ImGui::PushFont(Menu::boldFont18);
	else if (size == FontSize::SIZE_16) ImGui::PushFont(Menu::boldFont16);
	else if (size == FontSize::SIZE_14) ImGui::PushFont(Menu::boldFont14);

	// Red Glitch Offset (slightly displaced)
	ImVec2 pos_red = ImVec2(cursorPos.x - (1 + (rand() % 3)), cursorPos.y - (rand() % 2));
	ImGui::SetCursorPos(pos_red);  // Set the cursor to the new position
	ImGui::TextColored(ImColor(235, 5, 90, 100 + (rand() % 60)), "%s", text);  // Red text

	// Cyan Glitch Offset (slightly displaced)
	ImVec2 pos_cyan = ImVec2(cursorPos.x + (1 + (rand() % 3)), cursorPos.y + (rand() % 2));
	ImGui::SetCursorPos(pos_cyan);  // Set the cursor to the new position
	ImGui::TextColored(ImColor(32, 217, 217, 100 + (rand() % 60)), "%s", text);  // Cyan text

	// Main Text (centered)
	ImGui::SetCursorPos(cursorPos);  // Reset cursor to original position
	ImGui::TextColored(ImColor(255, 255, 255), "%s", text);  // Main white text

	// Pop the custom font to restore the default
	ImGui::PopFont();
}

void Menu::GlitchText(const char* text, ImVec2 pos, int size)
{
	// Red Text
	ImVec2 pos_one = ImVec2(pos.x - (1 + (rand() % 3)), pos.y - (rand() % 2));
	ImGui::GetWindowDrawList()->AddText(Menu::fontBold, size, pos_one, ImColor(235, 5, 90, 100 + (rand() % 60)), text);

	// Cyan Text;
	ImVec2 pos_two = ImVec2(pos.x + (1 + (rand() % 3)), pos.y + (rand() % 2));
	ImGui::GetWindowDrawList()->AddText(Menu::fontBold, size, pos_two, ImColor(32, 217, 217, 100 + (rand() % 60)), text);

	// Real Text
	ImGui::GetWindowDrawList()->AddText(Menu::fontBold, size, pos, ImColor(255, 255, 255), text);
}

void Menu::VerticalSeparator(const char* str_id, float size, float thickness)
{
	// Get the current cursor position
	ImVec2 p1 = ImGui::GetCursorScreenPos();

	// Define the position for the separator (just use thickness for the vertical separator)
	ImVec2 p2 = ImVec2(p1.x + thickness, p1.y + size);

	// Define the color for the separator
	ImColor color = ImColor(settings::Menu_SeperatorColor[0], settings::Menu_SeperatorColor[1], settings::Menu_SeperatorColor[2], settings::Menu_SeperatorColor[3]);

	// Draw the vertical separator using ImGui's window draw list
	ImGui::GetWindowDrawList()->AddRectFilled(p1, p2, color);

	// Use an invisible button to reserve space, with no visible UI change
	ImGui::InvisibleButton(("###VerticalSeparator" + std::string(str_id)).c_str(), ImVec2(thickness, size));
}

void Menu::HorizontalSeparator(const char* str_id, float size, float thickness)
{
	if (size == 0)
	{
		size = ImGui::GetWindowSize().x - 40.f;
	}

	// Get the current cursor position
	ImVec2 p1 = ImGui::GetCursorScreenPos();

	// Define the position for the separator (just use thickness for the horizontal separator)
	ImVec2 p2 = ImVec2(p1.x + size, p1.y + thickness);

	// Define the color for the separator
	ImColor color = ImColor(settings::Menu_SeperatorColor[0], settings::Menu_SeperatorColor[1], settings::Menu_SeperatorColor[2], settings::Menu_SeperatorColor[3]);

	// Draw the horizontal separator using ImGui's window draw list
	ImGui::GetWindowDrawList()->AddRectFilled(p1, p2, color);

	// Now, use an invisible button to move the cursor by just the thickness
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + thickness);

	// Use an invisible button to reserve space, with no visible UI change
	ImGui::InvisibleButton(("###HorizontalSeparator_" + std::string(str_id)).c_str(), ImVec2(size, thickness));
}

void Menu::HorizontalSeparatorText(const char* text, FontSize font_size, float size)
{
	if (size == 0)
	{
		size = ImGui::GetWindowSize().x - 40.f;
	}

	if (font_size == FontSize::SIZE_28) ImGui::PushFont(Menu::font28);
	else if (font_size == FontSize::SIZE_26) ImGui::PushFont(Menu::font26);
	else if (font_size == FontSize::SIZE_24) ImGui::PushFont(Menu::font24);
	else if (font_size == FontSize::SIZE_22) ImGui::PushFont(Menu::font22);
	else if (font_size == FontSize::SIZE_20) ImGui::PushFont(Menu::font20);
	else if (font_size == FontSize::SIZE_18) ImGui::PushFont(Menu::font18);
	else if (font_size == FontSize::SIZE_16) ImGui::PushFont(Menu::font16);
	else if (font_size == FontSize::SIZE_14) ImGui::PushFont(Menu::font14);

	ImVec2 textSize = ImGui::CalcTextSize(text);
	float textWidth = textSize.x;

	float separatorWidth = (size - textWidth - 20.f) / 2;

	Menu::HorizontalSeparator(("1TextSeparator_" + std::string(text)).c_str(), separatorWidth);
	ImGui::SameLine();
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (textSize.y / 2));
	ImGui::Text(text);
	ImGui::SameLine();
	Menu::HorizontalSeparator(("2TextSeparator_" + std::string(text)).c_str(), separatorWidth);

	ImGui::PopFont();
}

bool Menu::Button(const char* label, ImVec2 size, FontSize font_size)
{
	if (size.x == 0)
	{
		size.x = ImGui::GetWindowSize().x - 40.f;
	}

	if (font_size == FontSize::SIZE_28) ImGui::PushFont(Menu::font28);
	else if (font_size == FontSize::SIZE_26) ImGui::PushFont(Menu::font26);
	else if (font_size == FontSize::SIZE_24) ImGui::PushFont(Menu::font24);
	else if (font_size == FontSize::SIZE_22) ImGui::PushFont(Menu::font22);
	else if (font_size == FontSize::SIZE_20) ImGui::PushFont(Menu::font20);
	else if (font_size == FontSize::SIZE_18) ImGui::PushFont(Menu::font18);
	else if (font_size == FontSize::SIZE_16) ImGui::PushFont(Menu::font16);
	else if (font_size == FontSize::SIZE_14) ImGui::PushFont(Menu::font14);

	bool result = ImGui::Button(label, size);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}

	ImGui::PopFont();

	return result;
}

static bool IsMouseButton(int button)
{
	return button == 1 || button == 2 || button == 4 || button == 5 || button == 6;
}

static bool IsKeyboardKey(int key)
{
	return !IsMouseButton(key);
}

void Menu::KeybindButton(const char* text, int& keybind, bool allowMouse, bool allowKeyboard, ImVec2 size, FontSize font_size)
{
	if (size.x == 0)
	{
		size.x = ImGui::GetWindowSize().x - 40.f;
	}

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);
	Menu::Text(text, font_size);
	ImGui::SameLine();

	const float w = 120.f; // Width of checkbox and keybind button
	const float space = size.x - font18->CalcTextSizeA(18, FLT_MAX, 0.0f, text).x - w;

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + space - 10);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5.f);

	int keys_size = IM_ARRAYSIZE(keys);
	char name[18];
	strncpy_s(name, keys[std::clamp(keybind, 0, keys_size)], 18);
	static std::map<const char*, bool> bindings;
	if (bindings[text])
	{
		ImGui::Button("[...]", ImVec2(120, 0));

		for (int i = 0; i < keys_size; i++)
		{
			int key = i;
			if (!Keys::IsKeyPressedOnce(i) || (!allowMouse && IsMouseButton(i)) || (!allowKeyboard && IsKeyboardKey(i))) continue;

			if (i == VK_ESCAPE) keybind = 0;
			else keybind = i;

			strncpy_s(name, keys[std::clamp(keybind, 0, keys_size)], 18);
			bindings[text] = false;
			isBindingKey = false;
			break;
		}
	}
	else
	{
		if (ImGui::Button(Keys::GetKeyName(keybind), ImVec2(120, 0)))
		{
			bindings[text] = true;
			isBindingKey = true;
		}
	}
}

bool Menu::TransparentButton(const char* text, ImVec2 btn_size, FontSize font_size)
{
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
	
	if (font_size == FontSize::SIZE_28) ImGui::PushFont(Menu::font28);
	else if (font_size == FontSize::SIZE_26) ImGui::PushFont(Menu::font26);
	else if (font_size == FontSize::SIZE_24) ImGui::PushFont(Menu::font24);
	else if (font_size == FontSize::SIZE_22) ImGui::PushFont(Menu::font22);
	else if (font_size == FontSize::SIZE_20) ImGui::PushFont(Menu::font20);
	else if (font_size == FontSize::SIZE_18) ImGui::PushFont(Menu::font18);
	else if (font_size == FontSize::SIZE_16) ImGui::PushFont(Menu::font16);
	else if (font_size == FontSize::SIZE_14) ImGui::PushFont(Menu::font14);

	bool result = ImGui::Button(text, btn_size);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}

	ImGui::PopFont();

	ImGui::PopStyleColor(3);

	return result;
}

bool Menu::MenuButton(const char* text, ImVec2 btn_size, FontSize font_size)
{
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(settings::Menu_ChildBackgroundColor[0], settings::Menu_ChildBackgroundColor[1], settings::Menu_ChildBackgroundColor[2], settings::Menu_ChildBackgroundColor[3]));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImClamp<float>(settings::Menu_ChildBackgroundColor[0] * 0.8f, 0.0f, 1.0f), ImClamp<float>(settings::Menu_ChildBackgroundColor[1] * 0.8f, 0.0f, 1.0f), ImClamp<float>(settings::Menu_ChildBackgroundColor[2] * 0.8f, 0.0f, 1.0f), settings::Menu_ChildBackgroundColor[3]));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImClamp<float>(settings::Menu_ChildBackgroundColor[0] * 0.6f, 0.0f, 1.0f), ImClamp<float>(settings::Menu_ChildBackgroundColor[1] * 0.6f, 0.0f, 1.0f), ImClamp<float>(settings::Menu_ChildBackgroundColor[2] * 0.6f, 0.0f, 1.0f), settings::Menu_ChildBackgroundColor[3]));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(settings::Menu_OutlineColor[0], settings::Menu_OutlineColor[1], settings::Menu_OutlineColor[2], settings::Menu_OutlineColor[3]));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, settings::Menu_WindowRounding);

	if (font_size == FontSize::SIZE_28) ImGui::PushFont(Menu::font28);
	else if (font_size == FontSize::SIZE_26) ImGui::PushFont(Menu::font26);
	else if (font_size == FontSize::SIZE_24) ImGui::PushFont(Menu::font24);
	else if (font_size == FontSize::SIZE_22) ImGui::PushFont(Menu::font22);
	else if (font_size == FontSize::SIZE_20) ImGui::PushFont(Menu::font20);
	else if (font_size == FontSize::SIZE_18) ImGui::PushFont(Menu::font18);
	else if (font_size == FontSize::SIZE_16) ImGui::PushFont(Menu::font16);
	else if (font_size == FontSize::SIZE_14) ImGui::PushFont(Menu::font14);

	bool result = ImGui::Button(text, btn_size);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}

	ImGui::PopFont();

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(4);

	return result;
}

bool Menu::DetachButton(const char* text, ImVec2 btn_size, FontSize font_size)
{
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(settings::Menu_DetachButtonColor[0], settings::Menu_DetachButtonColor[1], settings::Menu_DetachButtonColor[2], settings::Menu_DetachButtonColor[3]));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImClamp<float>(settings::Menu_DetachButtonColor[0] * 0.8f, 0.0f, 1.0f), ImClamp<float>(settings::Menu_DetachButtonColor[1] * 0.8f, 0.0f, 1.0f), ImClamp<float>(settings::Menu_DetachButtonColor[2] * 0.8f, 0.0f, 1.0f), settings::Menu_DetachButtonColor[3]));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImClamp<float>(settings::Menu_DetachButtonColor[0] * 0.6f, 0.0f, 1.0f), ImClamp<float>(settings::Menu_DetachButtonColor[1] * 0.6f, 0.0f, 1.0f), ImClamp<float>(settings::Menu_DetachButtonColor[2] * 0.6f, 0.0f, 1.0f), settings::Menu_DetachButtonColor[3]));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(settings::Menu_OutlineColor[0], settings::Menu_OutlineColor[1], settings::Menu_OutlineColor[2], settings::Menu_OutlineColor[3]));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, settings::Menu_WindowRounding);

	if (font_size == FontSize::SIZE_28) ImGui::PushFont(Menu::font28);
	else if (font_size == FontSize::SIZE_26) ImGui::PushFont(Menu::font26);
	else if (font_size == FontSize::SIZE_24) ImGui::PushFont(Menu::font24);
	else if (font_size == FontSize::SIZE_22) ImGui::PushFont(Menu::font22);
	else if (font_size == FontSize::SIZE_20) ImGui::PushFont(Menu::font20);
	else if (font_size == FontSize::SIZE_18) ImGui::PushFont(Menu::font18);
	else if (font_size == FontSize::SIZE_16) ImGui::PushFont(Menu::font16);
	else if (font_size == FontSize::SIZE_14) ImGui::PushFont(Menu::font14);

	bool result = ImGui::Button(text, btn_size);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}

	ImGui::PopFont();

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(4);

	return result;
}

void Menu::ToggleWithKeybind(bool* enabled, int& keybind, ImVec2 size)
{
	if (size.x == 0)
	{
		size.x = ImGui::GetContentRegionAvail().x;
	}

	int keys_size = IM_ARRAYSIZE(keys);
	static bool binding = false;
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (!window || window->SkipItems)
		return;

	const float rowHeight = 26.0f;
	const ImVec2 pos = ImGui::GetCursorScreenPos();
	const ImVec2 rowMax(pos.x + size.x, pos.y + rowHeight);
	ImGui::InvisibleButton("##module_bind", ImVec2(size.x, rowHeight));
	if (ImGui::IsItemHovered())
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
	{
		binding = true;
		isBindingKey = true;
	}

	if (binding)
	{
		for (int i = 0; i < keys_size; i++)
		{
			if (!Keys::IsKeyPressedOnce(i)) continue;

			if (i == VK_ESCAPE) keybind = 0;
			else keybind = i;

			binding = false;
			isBindingKey = false;
			break;
		}
	}

	const std::string keyText = binding ? "[...]" : PrettyKeyNameLocal(keybind);
	ImFont* font = Menu::font18 ? Menu::font18 : ImGui::GetFont();
	const float fontSize = 18.0f;
	const ImU32 labelColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.45f, 0.45f, 0.49f, 1.0f));
	const ImU32 valueColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.56f, 0.56f, 0.60f, 1.0f));
	const ImVec2 valueSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, keyText.c_str());

	window->DrawList->AddText(font, fontSize, pos, labelColor, "Bind:");
	window->DrawList->AddText(font, fontSize, ImVec2(rowMax.x - valueSize.x, pos.y), valueColor, keyText.c_str());
}

bool Menu::Slider(const char* label, int* value, int min, int max, ImVec2 size, const char* format, ImGuiSliderFlags flags)
{
	if (size.x == 0)
	{
		size.x = ImGui::GetContentRegionAvail().x;
	}

	ImGuiDataType data_type = ImGuiDataType_S32;
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	const float width = std::max(90.0f, size.x);
	const ImVec2 pos = window->DC.CursorPos;
	const ImRect frame_bb(ImVec2(pos.x, pos.y + 21.0f), ImVec2(pos.x + width, pos.y + 39.0f));
	const ImRect total_bb(pos, ImVec2(pos.x + width, pos.y + 43.0f));

	const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
	ImGui::ItemSize(total_bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
		return false;

	if (format == NULL)
		format = ImGui::DataTypeGetInfo(data_type)->PrintFmt;

	const bool hovered = ImGui::ItemHoverable(frame_bb, id);
	bool temp_input_is_active = temp_input_allowed && ImGui::TempInputIsActive(id);
	if (!temp_input_is_active)
	{
		// Tabbing or CTRL-clicking on Slider turns it into an input box
		const bool input_requested_by_tabbing = temp_input_allowed && (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_FocusedByTabbing) != 0;
		const bool clicked = hovered && ImGui::IsMouseClicked(0, id);
		const bool make_active = (input_requested_by_tabbing || clicked || g.NavActivateId == id || g.NavActivateInputId == id);
		if (make_active && clicked)
			ImGui::SetKeyOwner(ImGuiKey_MouseLeft, id);
		if (make_active && temp_input_allowed)
			if (input_requested_by_tabbing || (clicked && g.IO.KeyCtrl) || g.NavActivateInputId == id)
				temp_input_is_active = true;

		if (make_active && !temp_input_is_active)
		{
			ImGui::SetActiveID(id, window);
			ImGui::SetFocusID(id, window);
			ImGui::FocusWindow(window);
			g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
		}
	}

	if (temp_input_is_active)
	{
		const bool is_clamp_input = (flags & ImGuiSliderFlags_AlwaysClamp) != 0;
		return ImGui::TempInputScalar(frame_bb, id, label, data_type, value, format, is_clamp_input ? &min : NULL, is_clamp_input ? &max : NULL);
	}

	ImRect grab_bb;
	const bool value_changed = ImGui::SliderBehavior(frame_bb, id, data_type, value, &min, &max, format, flags, &grab_bb);
	if (value_changed)
		ImGui::MarkItemEdited(id);
	if (hovered)
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

	char value_buf[64];
	const char* value_buf_end = value_buf + ImGui::DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, value, format);
	const ImVec2 valueSize = ImGui::CalcTextSize(value_buf);
	const char* displayLabel = StringUtils::ExtractBeforeDoubleHash(label);
	ImFont* sliderFont = Menu::font18 ? Menu::font18 : ImGui::GetFont();
	const float labelSize = 18.0f;
	window->DrawList->AddText(sliderFont, labelSize, pos, ImGui::GetColorU32(ImGuiCol_Text), displayLabel);
	window->DrawList->AddText(sliderFont, labelSize, ImVec2(pos.x + width - valueSize.x, pos.y), ImGui::GetColorU32(ImGuiCol_TextDisabled), value_buf, value_buf_end);

	ImGui::RenderNavHighlight(frame_bb, id);

	const float trackHeight = 4.0f;
	const ImVec2 trackMin(pos.x, pos.y + 29.0f);
	const ImVec2 trackMax(pos.x + width, trackMin.y + trackHeight);
	const float range = static_cast<float>(max - min);
	const float t = range == 0.0f ? 0.0f : std::clamp(static_cast<float>(*value - min) / range, 0.0f, 1.0f);
	const ImVec2 fillMax(trackMin.x + width * t, trackMax.y);
	window->DrawList->AddRectFilled(trackMin, trackMax, ImGui::ColorConvertFloat4ToU32(ImVec4(0.085f, 0.085f, 0.100f, 1.0f)), 2.0f);
	window->DrawList->AddRectFilled(trackMin, fillMax, SliderAccent(1.0f, g.ActiveId == id ? 1.0f : 0.88f), 2.0f);

	const ImVec2 knobCenter(fillMax.x, trackMin.y + trackHeight * 0.5f);
	if (g.ActiveId == id)
		window->DrawList->AddCircleFilled(knobCenter, 10.0f, SliderAccent(0.24f, 1.0f));
	window->DrawList->AddCircleFilled(knobCenter, 6.0f, SliderAccent(1.0f, g.ActiveId == id ? 1.0f : 0.92f));

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
	return value_changed;
}

bool Menu::Slider(const char* label, float* value, float min, float max, ImVec2 size, const char* format, ImGuiSliderFlags flags)
{
	if (size.x == 0)
	{
		size.x = ImGui::GetContentRegionAvail().x;
	}

	ImGuiDataType data_type = ImGuiDataType_Float;
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	const float width = std::max(90.0f, size.x);
	const ImVec2 pos = window->DC.CursorPos;
	const ImRect frame_bb(ImVec2(pos.x, pos.y + 21.0f), ImVec2(pos.x + width, pos.y + 39.0f));
	const ImRect total_bb(pos, ImVec2(pos.x + width, pos.y + 43.0f));

	const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
	ImGui::ItemSize(total_bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
		return false;

	if (format == NULL)
		format = ImGui::DataTypeGetInfo(data_type)->PrintFmt;

	const bool hovered = ImGui::ItemHoverable(frame_bb, id);
	bool temp_input_is_active = temp_input_allowed && ImGui::TempInputIsActive(id);
	if (!temp_input_is_active)
	{
		// Tabbing or CTRL-clicking on Slider turns it into an input box
		const bool input_requested_by_tabbing = temp_input_allowed && (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_FocusedByTabbing) != 0;
		const bool clicked = hovered && ImGui::IsMouseClicked(0, id);
		const bool make_active = (input_requested_by_tabbing || clicked || g.NavActivateId == id || g.NavActivateInputId == id);
		if (make_active && clicked)
			ImGui::SetKeyOwner(ImGuiKey_MouseLeft, id);
		if (make_active && temp_input_allowed)
			if (input_requested_by_tabbing || (clicked && g.IO.KeyCtrl) || g.NavActivateInputId == id)
				temp_input_is_active = true;

		if (make_active && !temp_input_is_active)
		{
			ImGui::SetActiveID(id, window);
			ImGui::SetFocusID(id, window);
			ImGui::FocusWindow(window);
			g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
		}
	}

	if (temp_input_is_active)
	{
		const bool is_clamp_input = (flags & ImGuiSliderFlags_AlwaysClamp) != 0;
		return ImGui::TempInputScalar(frame_bb, id, label, data_type, value, format, is_clamp_input ? &min : NULL, is_clamp_input ? &max : NULL);
	}

	ImRect grab_bb;
	const bool value_changed = ImGui::SliderBehavior(frame_bb, id, data_type, value, &min, &max, format, flags, &grab_bb);
	if (value_changed)
		ImGui::MarkItemEdited(id);
	if (hovered)
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

	char value_buf[64];
	const char* value_buf_end = value_buf + ImGui::DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, value, format);
	const ImVec2 valueSize = ImGui::CalcTextSize(value_buf);
	const char* displayLabel = StringUtils::ExtractBeforeDoubleHash(label);
	ImFont* sliderFont = Menu::font18 ? Menu::font18 : ImGui::GetFont();
	const float labelSize = 18.0f;
	window->DrawList->AddText(sliderFont, labelSize, pos, ImGui::GetColorU32(ImGuiCol_Text), displayLabel);
	window->DrawList->AddText(sliderFont, labelSize, ImVec2(pos.x + width - valueSize.x, pos.y), ImGui::GetColorU32(ImGuiCol_TextDisabled), value_buf, value_buf_end);

	ImGui::RenderNavHighlight(frame_bb, id);

	const float trackHeight = 4.0f;
	const ImVec2 trackMin(pos.x, pos.y + 29.0f);
	const ImVec2 trackMax(pos.x + width, trackMin.y + trackHeight);
	const float range = max - min;
	const float t = range == 0.0f ? 0.0f : std::clamp((*value - min) / range, 0.0f, 1.0f);
	const ImVec2 fillMax(trackMin.x + width * t, trackMax.y);
	window->DrawList->AddRectFilled(trackMin, trackMax, ImGui::ColorConvertFloat4ToU32(ImVec4(0.085f, 0.085f, 0.100f, 1.0f)), 2.0f);
	window->DrawList->AddRectFilled(trackMin, fillMax, SliderAccent(1.0f, g.ActiveId == id ? 1.0f : 0.88f), 2.0f);

	const ImVec2 knobCenter(fillMax.x, trackMin.y + trackHeight * 0.5f);
	if (g.ActiveId == id)
		window->DrawList->AddCircleFilled(knobCenter, 10.0f, SliderAccent(0.24f, 1.0f));
	window->DrawList->AddCircleFilled(knobCenter, 6.0f, SliderAccent(1.0f, g.ActiveId == id ? 1.0f : 0.92f));

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));

	return value_changed;
}

bool Menu::CheckboxBehavior(const char* label, bool* v)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

	// Pill toggle dimensions
	const float pillWidth = 32.0f;
	const float pillHeight = 16.0f;
	const ImVec2 pos = window->DC.CursorPos;
	const ImVec2 total_bb_max(pos.x + pillWidth + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), pos.y + label_size.y + style.FramePadding.y * 2.0f);
	const ImRect total_bb(pos, total_bb_max);
	ImGui::ItemSize(total_bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(total_bb, id))
	{
		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
		return false;
	}

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
	if (pressed)
	{
		*v = !(*v);
		ImGui::MarkItemEdited(id);
	}

	const ImVec2 pill_bb_max(pos.x + pillWidth, pos.y + pillHeight);
	const ImRect pill_bb(pos, pill_bb_max);
	ImGui::RenderNavHighlight(total_bb, id);
	
	// Pill background colors
	ImU32 pillBgColor;
	if (*v) {
		pillBgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.48f, 0.36f, 1.0f, 1.0f)); // #7A5CFF (purple when on)
	} else {
		pillBgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.16f, 0.16f, 0.21f, 1.0f)); // #2A2A35 (dark when off)
	}
	
	ImGui::RenderFrame(pill_bb.Min, pill_bb.Max, pillBgColor, true, pillHeight / 2.0f); // Rounded pill
	
	// Knob (white circle)
	const float knobSize = 12.0f;
	const float knobPadding = (pillHeight - knobSize) / 2.0f;
	
	// Animate knob position
	static std::map<ImGuiID, float> knobAnimations;
	float& animProgress = knobAnimations[id];
	float targetProgress = *v ? 1.0f : 0.0f;
	animProgress += (targetProgress - animProgress) * 0.15f; // Smooth lerp animation
	
	float knobX = pos.x + knobPadding + (animProgress * (pillWidth - knobSize - (knobPadding * 2.0f)));
	float knobY = pos.y + knobPadding;
	
	ImVec2 knobMin(knobX, knobY);
	ImVec2 knobMax(knobX + knobSize, knobY + knobSize);
	
	// Draw knob with white color
	ImU32 knobColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	window->DrawList->AddCircleFilled(ImVec2((knobMin.x + knobMax.x) / 2, (knobMin.y + knobMax.y) / 2), knobSize / 2.0f, knobColor);

	ImVec2 label_pos = ImVec2(pill_bb.Max.x + style.ItemInnerSpacing.x, pill_bb.Min.y + style.FramePadding.y);
	if (g.LogEnabled)
		ImGui::LogRenderedText(&label_pos, *v ? "[x]" : "[ ]");
	if (label_size.x > 0.0f)
		ImGui::RenderText(label_pos, label);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
	return pressed;
}

bool Menu::Checkbox(const char* label, bool* v, ImVec2 size)
{
	if (size.x == 0)
	{
		size.x = ImGui::GetWindowSize().x - 40.f;
	}

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);
	Menu::Text(label, FontSize::SIZE_18);
	ImGui::SameLine();

	const float w = 30.f; // Width of checkbox
	const float space = size.x - font18->CalcTextSizeA(18, FLT_MAX, 0.0f, label).x - w;

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + space - 10);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5.f);

	return CheckboxBehavior(("##" + std::string(label)).c_str(), v);
}

bool Menu::ColorEdit(const char* label, float* col, ImVec2 size, ImGuiColorEditFlags flags)
{
	if (size.x == 0)
	{
		size.x = ImGui::GetWindowSize().x - 40.f;
	}

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);
	Menu::Text(label, FontSize::SIZE_18);
	ImGui::SameLine();

	const float w = 30.f; // Width of preview
	const float space = size.x - font18->CalcTextSizeA(18, FLT_MAX, 0.0f, label).x - w;

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + space - 10);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5.f);

	return ImGui::ColorEdit4(("##" + std::string(label)).c_str(), col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | flags);
}

bool Menu::Dropdown(const char* label, const char* items[], int* item_current, int items_count, ImVec2 size)
{
	if (size.x == 0)
	{
		size.x = ImGui::GetWindowSize().x - 40.f;
	}

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);
	Menu::Text(label, FontSize::SIZE_18);
	ImGui::SameLine();

	const float w = 358.f; // Width of dropdown
	const float space = size.x - font18->CalcTextSizeA(18, FLT_MAX, 0.0f, label).x - w;

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + space - 10);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5.f);

	return ImGui::Combo(("##" + std::string(label)).c_str(), item_current, items, items_count);
}

void Menu::MoveCursorToCenter(bool checkInGame)
{
	if (checkInGame && SDK::minecraft->IsInGuiState())
		return;

	RECT clientRect;
	if (GetClientRect(handleWindow, &clientRect)) {
		// Calculate the center of the client area
		int clientCenterX = (clientRect.right - clientRect.left) / 2;
		int clientCenterY = (clientRect.bottom - clientRect.top) / 2;

		// Create a POINT to hold the center in client coordinates
		POINT clientCenterPoint = { clientCenterX, clientCenterY };

		// Convert client coordinates to screen coordinates
		ClientToScreen(handleWindow, &clientCenterPoint);

		// Move the cursor to the center of the client area
		SetCursorPos(clientCenterPoint.x, clientCenterPoint.y);
	}
}

void Menu::Shutdown()
{
	Menu::open = false;
	Menu::openHudEditor = false;
	Menu::RemoveHooks();
	if (Menu::handleDeviceContext && Menu::menuGLContext)
		wglMakeCurrent(Menu::handleDeviceContext, Menu::menuGLContext);
	Menu::CleanupIconTextures();
	wglMakeCurrent(Menu::handleDeviceContext, Menu::originalGLContext);
	wglDeleteContext(Menu::menuGLContext);
	//ImGui::DestroyContext(); // This is causing a crash
}

void Menu::PlaceHooks()
{
	Menu::Hook_wglSwapBuffers();
}

void Menu::RemoveHooks()
{
	Menu::Unhook_wndProc();
	Menu::Unhook_wglSwapBuffers();
}
