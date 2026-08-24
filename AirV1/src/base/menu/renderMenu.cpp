#include "menu.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <imgui/imgui_internal.h>
#include <windows.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include "configManager/configManager.h"
#include "configManager/settings.h"
#include "moduleManager/commonData.h"
#include "moduleManager/moduleManager.h"
#include "util/keys.h"

namespace
{
	constexpr float kReferenceWidth = 1920.0f;
	constexpr float kPanelWidth = 268.0f;
	constexpr float kPanelGap = 22.0f;
	constexpr float kPanelTop = 86.0f;
	constexpr float kHeaderHeight = 51.0f;
	constexpr float kRowHeight = 44.0f;
	constexpr float kExpandedHeight = 318.0f;
	constexpr float kPanelRounding = 2.0f;

	enum class PanelIcon
	{
		Combat,
		Misc,
		Movement,
		Visual,
		Menu,
		Configs
	};

	struct PanelSpec
	{
		const char* title;
		PanelIcon icon;
		std::vector<std::string> categories;
	};

	struct UiColors
	{
		ImU32 panel;
		ImU32 header;
		ImU32 row;
		ImU32 rowHover;
		ImU32 border;
		ImU32 text;
		ImU32 mutedText;
		ImU32 dimText;
		ImU32 shadow;
		ImU32 accent;
		ImU32 accentHover;
	};

	static std::unordered_map<std::string, float> g_moduleAnimation;
	static std::unordered_map<std::string, bool> g_moduleExpanded;
	static float g_menuSaturation = 0.70f;
	static float g_menuValue = 0.72f;
	static float g_copiedHue = settings::Menu_ThemeHue;
	static int g_descriptionMode = 0;
	static bool g_bindingMenuKey = false;
	static bool g_friendsOpen = false;
	static char g_searchBuffer[128] = "";

	ImU32 Color(const ImVec4& color)
	{
		return ImGui::ColorConvertFloat4ToU32(color);
	}

	ImVec4 AccentVec(float alpha = 1.0f, float saturation = g_menuSaturation, float value = g_menuValue)
	{
		ImVec4 color;
		ImGui::ColorConvertHSVtoRGB(settings::Menu_ThemeHue, saturation, value, color.x, color.y, color.z);
		color.w = alpha;
		return color;
	}

	ImU32 Accent(float alpha = 1.0f, float saturation = g_menuSaturation, float value = g_menuValue)
	{
		return Color(AccentVec(alpha, saturation, value));
	}

	UiColors Colors()
	{
		return {
			Color(ImVec4(0.025f, 0.025f, 0.030f, 0.96f)),
			Accent(0.72f, g_menuSaturation, std::max(0.28f, g_menuValue - 0.16f)),
			Color(ImVec4(0.040f, 0.040f, 0.047f, 0.64f)),
			Color(ImVec4(0.075f, 0.071f, 0.095f, 0.88f)),
			Color(ImVec4(0.0f, 0.0f, 0.0f, 0.78f)),
			Color(ImVec4(0.96f, 0.96f, 0.98f, 1.0f)),
			Color(ImVec4(0.56f, 0.56f, 0.60f, 1.0f)),
			Color(ImVec4(0.36f, 0.36f, 0.39f, 1.0f)),
			Color(ImVec4(0.0f, 0.0f, 0.0f, 0.36f)),
			Accent(),
			Accent(1.0f, g_menuSaturation, std::min(1.0f, g_menuValue + 0.12f))
		};
	}

	float UiScale()
	{
		const ImVec2 display = ImGui::GetIO().DisplaySize;
		return std::clamp(display.x / kReferenceWidth, 0.72f, 1.0f);
	}

	std::string ModuleId(ModuleBase* module)
	{
		return module->GetCategory() + "::" + module->GetName();
	}

	std::string PrettyKeyName(int key)
	{
		std::string name = Keys::GetKeyName(key);
		name.erase(std::remove(name.begin(), name.end(), '['), name.end());
		name.erase(std::remove(name.begin(), name.end(), ']'), name.end());

		if (name == "none") return "Unbound";
		if (name == "INS") return "Insert";
		if (name == "RSHIFT") return "Right Shift";
		if (name == "LSHIFT") return "Left Shift";
		if (name == "LCONTROL") return "Left Ctrl";
		if (name == "RCONTROL") return "Right Ctrl";
		if (name == "LMENU") return "Left Alt";
		if (name == "RMENU") return "Right Alt";
		if (name == "Space bar") return "Space";
		return name;
	}

	bool MouseInRect(const ImVec2& min, const ImVec2& max)
	{
		const ImVec2 mouse = ImGui::GetMousePos();
		return mouse.x >= min.x && mouse.x <= max.x && mouse.y >= min.y && mouse.y <= max.y;
	}

	std::string VisibleLabel(const char* label)
	{
		std::string visible = label ? label : "";
		const size_t idMarker = visible.find("##");
		if (idMarker != std::string::npos)
			visible.resize(idMarker);
		return visible;
	}

	std::string LowerAscii(std::string value)
	{
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});
		return value;
	}

	bool MatchesSearch(ModuleBase* module)
	{
		const std::string query = LowerAscii(g_searchBuffer);
		if (query.empty())
			return true;

		const std::string haystack = LowerAscii(module->GetName() + " " + module->GetCategory());
		return haystack.find(query) != std::string::npos;
	}

	void DrawChevron(ImDrawList* drawList, const ImVec2& center, float size, bool open, ImU32 color)
	{
		if (open)
		{
			drawList->AddLine(ImVec2(center.x - size, center.y - size * 0.35f), center, color, 1.7f);
			drawList->AddLine(center, ImVec2(center.x + size, center.y - size * 0.35f), color, 1.7f);
		}
		else
		{
			drawList->AddLine(ImVec2(center.x - size * 0.35f, center.y - size), center, color, 1.7f);
			drawList->AddLine(center, ImVec2(center.x - size * 0.35f, center.y + size), color, 1.7f);
		}
	}

	void DrawFallbackIcon(ImDrawList* drawList, const ImVec2& min, float scale, const char* text)
	{
		const UiColors c = Colors();
		ImFont* font = Menu::boldFont18 ? Menu::boldFont18 : ImGui::GetFont();
		const float fontSize = 17.0f * scale;
		const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);
		drawList->AddText(font, fontSize, ImVec2(min.x + (32.0f * scale - textSize.x) * 0.5f, min.y + (32.0f * scale - textSize.y) * 0.5f), c.text, text);
	}

	void DrawIcon(ImDrawList* drawList, PanelIcon icon, const ImVec2& min, float scale)
	{
		const float box = 32.0f * scale;
		const ImVec2 max(min.x + box, min.y + box);
		const float s = scale;

		ImTextureID textureID = nullptr;
		switch (icon)
		{
		case PanelIcon::Combat:
			textureID = Menu::combatIconTexture;
			break;
		case PanelIcon::Misc:
			textureID = Menu::utilityIconTexture;
			break;
		case PanelIcon::Movement:
			textureID = Menu::movementIconTexture;
			break;
		case PanelIcon::Visual:
			textureID = Menu::visualIconTexture;
			break;
		default:
			break;
		}

		if (textureID)
		{
			const ImVec2 texMin(min.x + 2.0f * s, min.y + 2.0f * s);
			const ImVec2 texMax(max.x - 2.0f * s, max.y - 2.0f * s);
			drawList->AddImage(textureID, texMin, texMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), Color(ImVec4(1, 1, 1, 1)));
			return;
		}

		if (icon == PanelIcon::Menu)
		{
			const ImVec2 center(min.x + box * 0.5f, min.y + box * 0.5f);
			const ImU32 white = Color(ImVec4(0.98f, 0.98f, 1.0f, 1.0f));
			for (int i = -1; i <= 1; ++i)
			{
				const float y = center.y + i * 6.0f * s;
				drawList->AddLine(ImVec2(center.x - 10.0f * s, y), ImVec2(center.x + 10.0f * s, y), white, 2.6f * s);
			}
			return;
		}

		if (icon == PanelIcon::Configs)
		{
			const ImVec2 center(min.x + box * 0.5f, min.y + box * 0.5f);
			const ImU32 white = Color(ImVec4(0.98f, 0.98f, 1.0f, 1.0f));
			drawList->AddRect(ImVec2(center.x - 10.5f * s, center.y - 6.0f * s), ImVec2(center.x + 11.0f * s, center.y + 8.0f * s), white, 2.0f * s, 0, 2.1f * s);
			drawList->AddLine(ImVec2(center.x - 9.0f * s, center.y - 6.0f * s), ImVec2(center.x - 3.0f * s, center.y - 10.0f * s), white, 2.0f * s);
			drawList->AddLine(ImVec2(center.x - 3.0f * s, center.y - 10.0f * s), ImVec2(center.x + 3.0f * s, center.y - 10.0f * s), white, 2.0f * s);
			return;
		}

		switch (icon)
		{
		case PanelIcon::Combat:
			DrawFallbackIcon(drawList, min, scale, "C");
			break;
		case PanelIcon::Misc:
			DrawFallbackIcon(drawList, min, scale, "U");
			break;
		case PanelIcon::Movement:
			DrawFallbackIcon(drawList, min, scale, "M");
			break;
		case PanelIcon::Visual:
			DrawFallbackIcon(drawList, min, scale, "V");
			break;
		default:
			break;
		}
	}

	void DrawPanelChrome(ImDrawList* drawList, const ImVec2& pos, const ImVec2& size, const char* title, PanelIcon icon, float scale)
	{
		const UiColors c = Colors();
		const float header = kHeaderHeight * scale;
		const float rounding = kPanelRounding * scale;

		drawList->AddRectFilled(ImVec2(pos.x + 5.0f * scale, pos.y + 5.0f * scale), ImVec2(pos.x + size.x + 5.0f * scale, pos.y + size.y + 5.0f * scale), c.shadow, rounding);
		drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), c.panel, rounding);
		drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), c.border, rounding, 0, 1.0f);
		drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + header), c.header, rounding, ImDrawFlags_RoundCornersTop);

		DrawIcon(drawList, icon, ImVec2(pos.x + 12.0f * scale, pos.y + 9.5f * scale), scale);

		const float titleSize = 21.0f * scale;
		ImFont* titleFont = Menu::boldFont18 ? Menu::boldFont18 : ImGui::GetFont();
		const ImVec2 titleText = titleFont->CalcTextSizeA(titleSize, FLT_MAX, 0.0f, title);
		drawList->AddText(titleFont, titleSize, ImVec2(pos.x + (size.x - titleText.x) * 0.5f, pos.y + (header - titleText.y) * 0.5f), c.text, title);
	}

	void PushReferenceStyle()
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.96f, 0.96f, 0.98f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.43f, 0.43f, 0.47f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.030f, 0.030f, 0.036f, 0.98f));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.045f, 0.045f, 0.052f, 0.98f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
		ImGui::PushStyleColor(ImGuiCol_Button, AccentVec(0.72f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, AccentVec(0.92f, g_menuSaturation, std::min(1.0f, g_menuValue + 0.13f)));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, AccentVec(1.0f, g_menuSaturation, std::max(0.35f, g_menuValue - 0.08f)));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.070f, 0.070f, 0.082f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.100f, 0.090f, 0.135f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, AccentVec(0.75f));
		ImGui::PushStyleColor(ImGuiCol_Header, AccentVec(0.76f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, AccentVec(0.90f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, AccentVec(1.0f));
		ImGui::PushStyleColor(ImGuiCol_CheckMark, AccentVec(1.0f, 0.85f, 0.92f));
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, AccentVec(1.0f));
		ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, AccentVec(1.0f, g_menuSaturation, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.035f, 0.035f, 0.042f, 0.98f));

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, kPanelRounding);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, kPanelRounding);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 2.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 4.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 7.0f));
	}

	void PopReferenceStyle()
	{
		ImGui::PopStyleVar(7);
		ImGui::PopStyleColor(18);
	}

	void UpdateAnimations(const std::vector<std::unique_ptr<ModuleBase>>& modules)
	{
		const float rate = std::min(1.0f, ImGui::GetIO().DeltaTime * 14.0f);
		for (const auto& module : modules)
		{
			const std::string id = ModuleId(module.get());
			const float target = g_moduleExpanded[id] ? 1.0f : 0.0f;
			float& progress = g_moduleAnimation[id];
			progress += (target - progress) * rate;
		}
	}

	std::vector<ModuleBase*> ModulesForPanel(const PanelSpec& spec, std::vector<std::unique_ptr<ModuleBase>>& modules)
	{
		std::vector<ModuleBase*> result;
		for (auto& module : modules)
		{
			if (std::find(spec.categories.begin(), spec.categories.end(), module->GetCategory()) != spec.categories.end())
			{
				if (MatchesSearch(module.get()))
					result.push_back(module.get());
			}
		}
		return result;
	}

	float PanelHeightForModules(const std::vector<ModuleBase*>& modules, float scale, float maxHeight)
	{
		float height = kHeaderHeight * scale + 8.0f * scale;
		for (ModuleBase* module : modules)
		{
			const float progress = g_moduleAnimation[ModuleId(module)];
			height += kRowHeight * scale;
			if (progress > 0.01f)
				height += kExpandedHeight * scale * progress;
		}
		height += 8.0f * scale;
		return std::min(height, maxHeight);
	}

	void RenderExpandedModule(ModuleBase* module, const ImVec2& pos, const ImVec2& size, float progress)
	{
		if (progress <= 0.02f)
			return;

		const UiColors c = Colors();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 max(pos.x + size.x, pos.y + size.y);
		drawList->AddRectFilled(pos, max, Color(ImVec4(0.050f, 0.050f, 0.060f, 0.96f)), 2.0f);
		drawList->AddRectFilled(pos, ImVec2(pos.x + 3.0f, max.y), c.accent, 2.0f, ImDrawFlags_RoundCornersLeft);

		if (progress < 0.45f)
			return;

		ImGui::SetCursorScreenPos(ImVec2(pos.x + 8.0f, pos.y + 7.0f));
		PushReferenceStyle();
		ImGui::BeginChild(("##expanded_" + ModuleId(module)).c_str(), ImVec2(size.x - 16.0f, size.y - 13.0f), false, ImGuiWindowFlags_NoBackground);
		ImGui::SetCursorPos(ImVec2(2.0f, 2.0f));
		module->RenderMenu();
		ImGui::EndChild();
		PopReferenceStyle();
	}

	void RenderModulePanel(const PanelSpec& spec, const ImVec2& pos, float width, float maxHeight, std::vector<std::unique_ptr<ModuleBase>>& modules, float scale)
	{
		const std::vector<ModuleBase*> panelModules = ModulesForPanel(spec, modules);
		const float height = PanelHeightForModules(panelModules, scale, maxHeight);
		const UiColors c = Colors();

		ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

		const std::string windowName = "AirV1ClickPanel_" + std::string(spec.title);
		ImGui::Begin(windowName.c_str(), nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBackground);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 windowPos = ImGui::GetWindowPos();
		DrawPanelChrome(drawList, windowPos, ImVec2(width, height), spec.title, spec.icon, scale);

		float y = windowPos.y + kHeaderHeight * scale + 4.0f * scale;
		for (ModuleBase* module : panelModules)
		{
			const std::string id = ModuleId(module);
			const float progress = g_moduleAnimation[id];
			const bool expanded = progress > 0.12f || g_moduleExpanded[id];
			const bool enabled = module->IsEnabled();
			const ImVec2 rowMin(windowPos.x + 10.0f * scale, y);
			const ImVec2 rowMax(windowPos.x + width - 10.0f * scale, y + kRowHeight * scale);
			const bool hovered = MouseInRect(rowMin, rowMax);

			ImU32 rowColor = c.row;
			if (expanded || enabled)
				rowColor = Accent(0.88f, g_menuSaturation, std::max(0.46f, g_menuValue - 0.08f));
			else if (hovered)
				rowColor = c.rowHover;

			drawList->AddRectFilled(rowMin, rowMax, rowColor, 1.5f * scale);

			ImFont* rowFont = Menu::font18 ? Menu::font18 : ImGui::GetFont();
			const float rowFontSize = 20.0f * scale;
			const std::string moduleName = module->GetName();
			const ImVec2 textSize = rowFont->CalcTextSizeA(rowFontSize, FLT_MAX, 0.0f, moduleName.c_str());
			const ImU32 textColor = expanded || enabled ? c.text : c.mutedText;
			drawList->AddText(rowFont, rowFontSize, ImVec2(rowMin.x + 12.0f * scale, rowMin.y + (kRowHeight * scale - textSize.y) * 0.5f), textColor, moduleName.c_str());

			DrawChevron(drawList, ImVec2(rowMax.x - 16.0f * scale, rowMin.y + kRowHeight * scale * 0.5f), 5.2f * scale, expanded, expanded ? c.text : c.dimText);

			ImGui::SetCursorScreenPos(rowMin);
			ImGui::InvisibleButton(("##row_" + id).c_str(), ImVec2(rowMax.x - rowMin.x, rowMax.y - rowMin.y));
			if (ImGui::IsItemHovered())
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
				module->Toggle();
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
				g_moduleExpanded[id] = !g_moduleExpanded[id];

			y += kRowHeight * scale;
			if (progress > 0.01f)
			{
				const float expandedHeight = kExpandedHeight * scale * progress;
				RenderExpandedModule(module, ImVec2(windowPos.x + 10.0f * scale, y), ImVec2(width - 20.0f * scale, expandedHeight), progress);
				y += expandedHeight;
			}
		}

		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
	}

	bool FlatButton(const char* label, const ImVec2& pos, const ImVec2& size, float scale)
	{
		const UiColors c = Colors();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 max(pos.x + size.x, pos.y + size.y);
		const bool hovered = MouseInRect(pos, max);
		const std::string visible = VisibleLabel(label);
		drawList->AddRectFilled(pos, max, hovered ? c.accentHover : Color(ImVec4(0.048f, 0.048f, 0.056f, 1.0f)), 1.5f * scale);
		drawList->AddRect(pos, max, Color(ImVec4(0.0f, 0.0f, 0.0f, 0.55f)), 1.5f * scale);

		ImFont* font = Menu::boldFont16 ? Menu::boldFont16 : ImGui::GetFont();
		const float fontSize = 16.0f * scale;
		const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, visible.c_str());
		drawList->AddText(font, fontSize, ImVec2(pos.x + (size.x - textSize.x) * 0.5f, pos.y + (size.y - textSize.y) * 0.5f), c.text, visible.c_str());

		ImGui::SetCursorScreenPos(pos);
		ImGui::InvisibleButton((std::string("##button_") + label).c_str(), size);
		if (ImGui::IsItemHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		return ImGui::IsItemClicked(ImGuiMouseButton_Left);
	}

	void BindKeyLine(const ImVec2& pos, float width, float scale)
	{
		const UiColors c = Colors();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImFont* font = Menu::font18 ? Menu::font18 : ImGui::GetFont();
		const float fontSize = 18.0f * scale;
		const std::string keyText = g_bindingMenuKey ? "[...]" : PrettyKeyName(settings::Menu_Keybind);
		const ImVec2 valueSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, keyText.c_str());

		drawList->AddText(font, fontSize, pos, c.dimText, "Menu Key:");
		const ImVec2 valuePos(pos.x + width - valueSize.x, pos.y);
		drawList->AddText(font, fontSize, valuePos, c.mutedText, keyText.c_str());

		ImGui::SetCursorScreenPos(ImVec2(valuePos.x - 10.0f * scale, valuePos.y - 4.0f * scale));
		ImGui::InvisibleButton("##bind_menu_key", ImVec2(valueSize.x + 16.0f * scale, valueSize.y + 8.0f * scale));
		if (ImGui::IsItemHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			g_bindingMenuKey = true;
			Menu::isBindingKey = true;
		}

		if (g_bindingMenuKey)
		{
			const int keyCount = IM_ARRAYSIZE(keys);
			for (int i = 1; i < keyCount; ++i)
			{
				if (!Keys::IsKeyPressedOnce(i))
					continue;

				settings::Menu_Keybind = (i == VK_ESCAPE) ? 0 : i;
				g_bindingMenuKey = false;
				Menu::isBindingKey = false;
				break;
			}
		}
	}

	void DescriptionLine(const ImVec2& pos, float width, float scale)
	{
		static const std::array<const char*, 3> descriptions = { "Search Bar", "Tooltips", "Off" };
		const UiColors c = Colors();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImFont* font = Menu::font18 ? Menu::font18 : ImGui::GetFont();
		const float fontSize = 18.0f * scale;
		const std::string value = descriptions[g_descriptionMode];
		const ImVec2 valueSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, value.c_str());

		drawList->AddText(font, fontSize, pos, c.dimText, "Descriptions:");
		const ImVec2 valuePos(pos.x + width - valueSize.x - 18.0f * scale, pos.y);
		drawList->AddText(font, fontSize, valuePos, c.mutedText, value.c_str());
		DrawChevron(drawList, ImVec2(pos.x + width - 7.0f * scale, pos.y + 10.0f * scale), 4.4f * scale, true, c.dimText);

		ImGui::SetCursorScreenPos(ImVec2(valuePos.x - 10.0f * scale, valuePos.y - 4.0f * scale));
		ImGui::InvisibleButton("##description_mode", ImVec2(width - (valuePos.x - pos.x), valueSize.y + 8.0f * scale));
		if (ImGui::IsItemHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			g_descriptionMode = (g_descriptionMode + 1) % static_cast<int>(descriptions.size());
	}

	void DrawHueStrip(ImDrawList* drawList, const ImVec2& min, const ImVec2& max)
	{
		const int steps = 96;
		for (int i = 0; i < steps; ++i)
		{
			const float x0 = min.x + (max.x - min.x) * (static_cast<float>(i) / steps);
			const float x1 = min.x + (max.x - min.x) * (static_cast<float>(i + 1) / steps);
			ImVec4 c0;
			ImVec4 c1;
			ImGui::ColorConvertHSVtoRGB(static_cast<float>(i) / steps, 1.0f, 1.0f, c0.x, c0.y, c0.z);
			ImGui::ColorConvertHSVtoRGB(static_cast<float>(i + 1) / steps, 1.0f, 1.0f, c1.x, c1.y, c1.z);
			c0.w = 1.0f;
			c1.w = 1.0f;
			drawList->AddRectFilledMultiColor(ImVec2(x0, min.y), ImVec2(x1 + 1.0f, max.y), Color(c0), Color(c1), Color(c1), Color(c0));
		}
	}

	void ColorPickerBlock(const ImVec2& pos, float width, float scale)
	{
		const UiColors c = Colors();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImFont* font = Menu::font18 ? Menu::font18 : ImGui::GetFont();
		const float fontSize = 18.0f * scale;

		drawList->AddText(font, fontSize, pos, c.dimText, "Menu Color");
		drawList->AddCircleFilled(ImVec2(pos.x + width - 8.0f * scale, pos.y + 10.5f * scale), 8.0f * scale, c.accent);

		const ImVec2 squareMin(pos.x + 10.0f * scale, pos.y + 36.0f * scale);
		const ImVec2 squareMax(pos.x + width - 10.0f * scale, squareMin.y + 152.0f * scale);
		ImVec4 hue;
		ImGui::ColorConvertHSVtoRGB(settings::Menu_ThemeHue, 1.0f, 1.0f, hue.x, hue.y, hue.z);
		hue.w = 1.0f;
		drawList->AddRectFilledMultiColor(squareMin, squareMax, Color(ImVec4(1, 1, 1, 1)), Color(hue), Color(ImVec4(0, 0, 0, 1)), Color(ImVec4(0, 0, 0, 1)));
		drawList->AddRect(squareMin, squareMax, Color(ImVec4(0.0f, 0.0f, 0.0f, 0.72f)), 1.0f);

		ImGui::SetCursorScreenPos(squareMin);
		ImGui::InvisibleButton("##menu_sv_square", ImVec2(squareMax.x - squareMin.x, squareMax.y - squareMin.y));
		if (ImGui::IsItemActive())
		{
			const ImVec2 mouse = ImGui::GetMousePos();
			g_menuSaturation = std::clamp((mouse.x - squareMin.x) / (squareMax.x - squareMin.x), 0.0f, 1.0f);
			g_menuValue = std::clamp(1.0f - (mouse.y - squareMin.y) / (squareMax.y - squareMin.y), 0.25f, 1.0f);
		}

		const ImVec2 pickerDot(squareMin.x + g_menuSaturation * (squareMax.x - squareMin.x), squareMin.y + (1.0f - g_menuValue) * (squareMax.y - squareMin.y));
		drawList->AddCircleFilled(pickerDot, 5.0f * scale, Color(ImVec4(0, 0, 0, 0.80f)));
		drawList->AddCircle(pickerDot, 5.0f * scale, c.text, 16, 1.2f * scale);

		const ImVec2 hueMin(squareMin.x, squareMax.y + 16.0f * scale);
		const ImVec2 hueMax(squareMax.x, hueMin.y + 20.0f * scale);
		DrawHueStrip(drawList, hueMin, hueMax);
		drawList->AddRect(hueMin, hueMax, Color(ImVec4(0.0f, 0.0f, 0.0f, 0.72f)), 1.0f);

		ImGui::SetCursorScreenPos(hueMin);
		ImGui::InvisibleButton("##menu_hue_strip", ImVec2(hueMax.x - hueMin.x, hueMax.y - hueMin.y));
		if (ImGui::IsItemActive())
		{
			const ImVec2 mouse = ImGui::GetMousePos();
			settings::Menu_ThemeHue = std::clamp((mouse.x - hueMin.x) / (hueMax.x - hueMin.x), 0.0f, 1.0f);
		}
		const float markerX = hueMin.x + settings::Menu_ThemeHue * (hueMax.x - hueMin.x);
		drawList->AddLine(ImVec2(markerX, hueMin.y), ImVec2(markerX, hueMax.y), Color(ImVec4(0, 0, 0, 0.85f)), 3.0f * scale);
		drawList->AddLine(ImVec2(markerX + 2.0f * scale, hueMin.y), ImVec2(markerX + 2.0f * scale, hueMax.y), Color(ImVec4(1, 1, 1, 0.65f)), 1.0f * scale);

		const ImVec2 previewMin(hueMin.x, hueMax.y + 12.0f * scale);
		const ImVec2 previewMax(hueMax.x, previewMin.y + 19.0f * scale);
		drawList->AddRectFilled(previewMin, previewMax, c.accent, 1.0f);
		drawList->AddRect(previewMin, previewMax, Color(ImVec4(0.0f, 0.0f, 0.0f, 0.72f)), 1.0f);

		const ImVec2 copyPos(squareMin.x, previewMax.y + 13.0f * scale);
		const ImVec2 buttonSize((width - 42.0f * scale) * 0.5f, 31.0f * scale);
		if (FlatButton("Copy", copyPos, buttonSize, scale))
			g_copiedHue = settings::Menu_ThemeHue;
		if (FlatButton("Paste", ImVec2(copyPos.x + buttonSize.x + 18.0f * scale, copyPos.y), buttonSize, scale))
			settings::Menu_ThemeHue = g_copiedHue;
	}

	void RenderMenuPanel(const ImVec2& pos, float width, float scale)
	{
		const float height = 464.0f * scale;
		ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
		ImGui::Begin("AirV1ClickPanel_Menu", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBackground);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 windowPos = ImGui::GetWindowPos();
		DrawPanelChrome(drawList, windowPos, ImVec2(width, height), "Menu", PanelIcon::Menu, scale);

		const float contentX = windowPos.x + 12.0f * scale;
		const float contentWidth = width - 24.0f * scale;
		float y = windowPos.y + kHeaderHeight * scale + 18.0f * scale;
		BindKeyLine(ImVec2(contentX, y), contentWidth, scale);
		y += 43.0f * scale;
		DescriptionLine(ImVec2(contentX, y), contentWidth, scale);
		y += 43.0f * scale;
		ColorPickerBlock(ImVec2(contentX, y), contentWidth, scale);

		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
	}

	void RenderConfigPanel(const ImVec2& pos, float width, float scale)
	{
		const float height = 322.0f * scale;
		ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
		ImGui::Begin("AirV1ClickPanel_Configs", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBackground);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 windowPos = ImGui::GetWindowPos();
		DrawPanelChrome(drawList, windowPos, ImVec2(width, height), "Configs", PanelIcon::Configs, scale);

		const float contentX = windowPos.x + 12.0f * scale;
		const float contentWidth = width - 24.0f * scale;
		float y = windowPos.y + kHeaderHeight * scale + 13.0f * scale;

		static char configName[128] = "";
		ImFont* labelFont = Menu::font16 ? Menu::font16 : ImGui::GetFont();
		drawList->AddText(labelFont, 16.0f * scale, ImVec2(contentX, y), Color(ImVec4(0.56f, 0.56f, 0.60f, 1.0f)), "Config Name");
		y += 21.0f * scale;

		const float saveWidth = 70.0f * scale;
		const float inputWidth = contentWidth - saveWidth - 8.0f * scale;
		ImGui::SetCursorScreenPos(ImVec2(contentX, y));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.96f, 0.96f, 0.98f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.060f, 0.060f, 0.070f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.080f, 0.075f, 0.100f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.090f, 0.080f, 0.120f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f * scale);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f * scale, 6.0f * scale));
		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputTextWithHint("##config_name", "Name", configName, IM_ARRAYSIZE(configName));
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(4);

		if (FlatButton("SAVE##typed_config", ImVec2(contentX + inputWidth + 8.0f * scale, y), ImVec2(saveWidth, 30.0f * scale), scale) && std::strlen(configName) > 0)
		{
			configmanager::SaveConfig(configName);
		}

		y += 45.0f * scale;
		drawList->AddText(labelFont, 16.0f * scale, ImVec2(contentX, y), Color(ImVec4(0.56f, 0.56f, 0.60f, 1.0f)), "Available Configs");
		y += 23.0f * scale;

		const ImVec2 listPos(contentX, y);
		const ImVec2 listSize(contentWidth, windowPos.y + height - y - 12.0f * scale);
		drawList->AddRectFilled(listPos, ImVec2(listPos.x + listSize.x, listPos.y + listSize.y), Color(ImVec4(0.035f, 0.035f, 0.042f, 0.78f)), 2.0f * scale);
		drawList->AddRect(listPos, ImVec2(listPos.x + listSize.x, listPos.y + listSize.y), Color(ImVec4(0, 0, 0, 0.55f)), 2.0f * scale);

		ImGui::SetCursorScreenPos(ImVec2(listPos.x + 6.0f * scale, listPos.y + 6.0f * scale));
		ImGui::BeginChild("##config_list", ImVec2(listSize.x - 12.0f * scale, listSize.y - 12.0f * scale), false, ImGuiWindowFlags_NoBackground);

		const std::vector<std::string> configs = configmanager::GetConfigList();
		if (configs.empty())
		{
			ImFont* emptyFont = Menu::font16 ? Menu::font16 : ImGui::GetFont();
			ImGui::GetWindowDrawList()->AddText(emptyFont, 16.0f * scale, ImGui::GetCursorScreenPos(), Color(ImVec4(0.43f, 0.43f, 0.47f, 1.0f)), "No saved configs");
			ImGui::Dummy(ImVec2(1.0f, 24.0f * scale));
		}
		else
		{
			ImDrawList* listDrawList = ImGui::GetWindowDrawList();
			ImFont* rowFont = Menu::font16 ? Menu::font16 : ImGui::GetFont();
			const float rowHeight = 30.0f * scale;
			const float loadWidth = 48.0f * scale;
			const float deleteWidth = 58.0f * scale;
			const float gap = 5.0f * scale;

			for (int i = 0; i < static_cast<int>(configs.size()); ++i)
			{
				const ImVec2 rowMin = ImGui::GetCursorScreenPos();
				const float rowWidth = ImGui::GetContentRegionAvail().x;
				const ImVec2 rowMax(rowMin.x + rowWidth, rowMin.y + rowHeight);
				listDrawList->AddRectFilled(rowMin, rowMax, Color(ImVec4(0.050f, 0.050f, 0.058f, 0.88f)), 1.5f * scale);

				std::string configText = configs[i];
				const float maxNameWidth = rowWidth - loadWidth - deleteWidth - gap * 3.0f - 10.0f * scale;
				while (!configText.empty() && rowFont->CalcTextSizeA(16.0f * scale, FLT_MAX, 0.0f, configText.c_str()).x > maxNameWidth)
				{
					configText.pop_back();
				}
				if (configText != configs[i] && configText.size() > 3)
				{
					configText.resize(configText.size() - 3);
					configText += "...";
				}

				listDrawList->AddText(rowFont, 16.0f * scale, ImVec2(rowMin.x + 8.0f * scale, rowMin.y + 7.0f * scale), Colors().text, configText.c_str());

				const float deleteX = rowMax.x - deleteWidth;
				const float loadX = deleteX - gap - loadWidth;
				if (FlatButton(("LOAD##config_" + std::to_string(i)).c_str(), ImVec2(loadX, rowMin.y + 3.0f * scale), ImVec2(loadWidth, rowHeight - 6.0f * scale), scale))
				{
					configmanager::LoadConfig(i);
					strncpy_s(configName, configs[i].c_str(), _TRUNCATE);
				}
				if (FlatButton(("DELETE##config_" + std::to_string(i)).c_str(), ImVec2(deleteX, rowMin.y + 3.0f * scale), ImVec2(deleteWidth, rowHeight - 6.0f * scale), scale))
				{
					configmanager::RemoveConfig(i);
				}

				ImGui::Dummy(ImVec2(rowWidth, rowHeight + 5.0f * scale));
			}
		}

		ImGui::EndChild();

		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
	}

	void DrawSearchIcon(ImDrawList* drawList, const ImVec2& center, float scale, ImU32 color)
	{
		drawList->AddCircle(center, 6.0f * scale, color, 20, 2.0f * scale);
		drawList->AddLine(
			ImVec2(center.x + 4.5f * scale, center.y + 4.5f * scale),
			ImVec2(center.x + 10.0f * scale, center.y + 10.0f * scale),
			color,
			2.0f * scale);
	}

	void DrawFriendsFallback(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, float scale, ImU32 color)
	{
		const ImVec2 center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
		drawList->AddCircleFilled(ImVec2(center.x, center.y - 5.5f * scale), 4.0f * scale, color);
		drawList->AddCircleFilled(ImVec2(center.x - 8.0f * scale, center.y - 3.0f * scale), 3.2f * scale, color);
		drawList->AddCircleFilled(ImVec2(center.x + 8.0f * scale, center.y - 3.0f * scale), 3.2f * scale, color);
		drawList->AddRectFilled(ImVec2(center.x - 9.0f * scale, center.y + 1.0f * scale), ImVec2(center.x + 9.0f * scale, center.y + 9.0f * scale), color, 5.0f * scale);
		drawList->AddRectFilled(ImVec2(center.x - 15.0f * scale, center.y + 2.0f * scale), ImVec2(center.x - 5.0f * scale, center.y + 8.5f * scale), color, 4.0f * scale);
		drawList->AddRectFilled(ImVec2(center.x + 5.0f * scale, center.y + 2.0f * scale), ImVec2(center.x + 15.0f * scale, center.y + 8.5f * scale), color, 4.0f * scale);
	}

	bool SquareIconButton(const char* id, const ImVec2& pos, const ImVec2& size, float scale, ImTextureID texture, bool friendsButton)
	{
		const UiColors c = Colors();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 max(pos.x + size.x, pos.y + size.y);
		const bool hovered = MouseInRect(pos, max);

		drawList->AddRectFilled(pos, max, hovered ? c.accentHover : c.accent, 7.0f * scale);
		drawList->AddRect(pos, max, Color(ImVec4(0.0f, 0.0f, 0.0f, 0.58f)), 7.0f * scale);

		const ImU32 iconColor = Color(ImVec4(1.0f, 1.0f, 1.0f, 0.96f));
		if (texture)
		{
			const float pad = 8.0f * scale;
			drawList->AddImage(texture, ImVec2(pos.x + pad, pos.y + pad), ImVec2(max.x - pad, max.y - pad), ImVec2(0, 0), ImVec2(1, 1), iconColor);
		}
		else if (friendsButton)
		{
			DrawFriendsFallback(drawList, pos, max, scale, iconColor);
		}
		else
		{
			DrawSearchIcon(drawList, ImVec2(pos.x + size.x * 0.46f, pos.y + size.y * 0.45f), scale, iconColor);
		}

		ImGui::SetCursorScreenPos(pos);
		ImGui::InvisibleButton(id, size);
		if (ImGui::IsItemHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		return ImGui::IsItemClicked(ImGuiMouseButton_Left);
	}

	void RenderBottomBar(float scale)
	{
		const ImVec2 display = ImGui::GetIO().DisplaySize;
		const float searchWidth = 400.0f * scale;
		const float buttonSize = 42.0f * scale;
		const float gap = 8.0f * scale;
		const float totalWidth = searchWidth + buttonSize + gap;
		const ImVec2 pos((display.x - totalWidth) * 0.5f, display.y - 58.0f * scale);

		ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(totalWidth, buttonSize), ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
		ImGui::Begin("AirV1BottomSearch", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBackground);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 windowPos = ImGui::GetWindowPos();
		const ImVec2 searchMin = windowPos;
		const ImVec2 searchMax(windowPos.x + searchWidth, windowPos.y + buttonSize);
		drawList->AddRectFilled(searchMin, searchMax, Color(ImVec4(0.025f, 0.025f, 0.030f, 0.98f)), 8.0f * scale);
		drawList->AddRect(searchMin, searchMax, Color(ImVec4(0.0f, 0.0f, 0.0f, 0.72f)), 8.0f * scale);

		const float innerPad = 12.0f * scale;
		const float searchButtonSize = 34.0f * scale;
		ImGui::SetCursorScreenPos(ImVec2(searchMin.x + innerPad, searchMin.y + 5.0f * scale));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f * scale);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 7.0f * scale));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.96f, 0.96f, 0.98f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.52f, 0.52f, 0.56f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
		ImGui::SetNextItemWidth(searchWidth - searchButtonSize - innerPad * 2.0f - 5.0f * scale);
		ImGui::InputTextWithHint("##module_search", "Search", g_searchBuffer, IM_ARRAYSIZE(g_searchBuffer));
		ImGui::PopStyleColor(5);
		ImGui::PopStyleVar(2);

		const ImVec2 searchButtonPos(searchMax.x - searchButtonSize - 4.0f * scale, searchMin.y + 4.0f * scale);
		SquareIconButton("##module_search_button", searchButtonPos, ImVec2(searchButtonSize, searchButtonSize), scale, nullptr, false);

		if (SquareIconButton("##friends_button", ImVec2(searchMax.x + gap, searchMin.y), ImVec2(buttonSize, buttonSize), scale, Menu::friendsIconTexture, true))
			g_friendsOpen = !g_friendsOpen;

		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
	}

	std::vector<std::string> ServerPlayerNames()
	{
		std::vector<std::string> names;
		for (const CommonData::PlayerData& player : CommonData::nativePlayerList)
		{
			if (player.name.empty() || player.name == CommonData::playerName)
				continue;

			if (std::find(names.begin(), names.end(), player.name) == names.end())
				names.push_back(player.name);
		}

		std::sort(names.begin(), names.end());
		return names;
	}

	void RenderFriendRow(const std::string& name, int index, bool online, float scale)
	{
		const UiColors c = Colors();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImFont* font = Menu::font16 ? Menu::font16 : ImGui::GetFont();
		const float rowHeight = 31.0f * scale;
		const ImVec2 rowMin = ImGui::GetCursorScreenPos();
		const float rowWidth = ImGui::GetContentRegionAvail().x;
		const ImVec2 rowMax(rowMin.x + rowWidth, rowMin.y + rowHeight);
		const bool isFriend = configmanager::IsFriend(name);

		drawList->AddRectFilled(rowMin, rowMax, Color(ImVec4(0.050f, 0.050f, 0.058f, 0.88f)), 1.5f * scale);
		drawList->AddText(font, 16.0f * scale, ImVec2(rowMin.x + 8.0f * scale, rowMin.y + 7.0f * scale), online ? c.text : c.mutedText, name.c_str());

		const float buttonWidth = 32.0f * scale;
		const ImVec2 buttonPos(rowMax.x - buttonWidth - 3.0f * scale, rowMin.y + 3.0f * scale);
		if (isFriend)
		{
			if (FlatButton(("X##friend_remove_" + std::to_string(index)).c_str(), buttonPos, ImVec2(buttonWidth, rowHeight - 6.0f * scale), scale))
				configmanager::RemoveFriend(name);
		}
		else
		{
			if (FlatButton(("+##friend_add_" + std::to_string(index)).c_str(), buttonPos, ImVec2(buttonWidth, rowHeight - 6.0f * scale), scale))
				configmanager::AddFriend(name);
		}

		ImGui::Dummy(ImVec2(rowWidth, rowHeight + 5.0f * scale));
	}

	void RenderFriendsWindow(float scale)
	{
		if (!g_friendsOpen)
			return;

		const ImVec2 display = ImGui::GetIO().DisplaySize;
		const ImVec2 size(292.0f * scale, 350.0f * scale);
		const ImVec2 pos((display.x - size.x) * 0.5f + 236.0f * scale, display.y - size.y - 106.0f * scale);

		ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(size, ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
		ImGui::Begin("AirV1FriendsWindow", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBackground);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 windowPos = ImGui::GetWindowPos();
		const UiColors c = Colors();
		drawList->AddRectFilled(ImVec2(windowPos.x + 5.0f * scale, windowPos.y + 5.0f * scale), ImVec2(windowPos.x + size.x + 5.0f * scale, windowPos.y + size.y + 5.0f * scale), c.shadow, 2.0f * scale);
		drawList->AddRectFilled(windowPos, ImVec2(windowPos.x + size.x, windowPos.y + size.y), c.panel, 2.0f * scale);
		drawList->AddRect(windowPos, ImVec2(windowPos.x + size.x, windowPos.y + size.y), c.border, 2.0f * scale);
		drawList->AddRectFilled(windowPos, ImVec2(windowPos.x + size.x, windowPos.y + kHeaderHeight * scale), c.header, 2.0f * scale, ImDrawFlags_RoundCornersTop);

		const ImVec2 iconMin(windowPos.x + 12.0f * scale, windowPos.y + 9.5f * scale);
		const ImVec2 iconMax(iconMin.x + 32.0f * scale, iconMin.y + 32.0f * scale);
		if (Menu::friendsIconTexture)
			drawList->AddImage(Menu::friendsIconTexture, iconMin, iconMax, ImVec2(0, 0), ImVec2(1, 1), Color(ImVec4(1, 1, 1, 1)));
		else
			DrawFriendsFallback(drawList, iconMin, iconMax, scale, c.text);

		ImFont* titleFont = Menu::boldFont18 ? Menu::boldFont18 : ImGui::GetFont();
		const float titleSize = 21.0f * scale;
		const ImVec2 titleText = titleFont->CalcTextSizeA(titleSize, FLT_MAX, 0.0f, "Friends");
		drawList->AddText(titleFont, titleSize, ImVec2(windowPos.x + (size.x - titleText.x) * 0.5f, windowPos.y + (kHeaderHeight * scale - titleText.y) * 0.5f), c.text, "Friends");

		if (FlatButton("X##close_friends", ImVec2(windowPos.x + size.x - 34.0f * scale, windowPos.y + 10.0f * scale), ImVec2(24.0f * scale, 24.0f * scale), scale))
			g_friendsOpen = false;

		const ImVec2 listPos(windowPos.x + 10.0f * scale, windowPos.y + kHeaderHeight * scale + 10.0f * scale);
		const ImVec2 listSize(size.x - 20.0f * scale, size.y - kHeaderHeight * scale - 20.0f * scale);
		drawList->AddRectFilled(listPos, ImVec2(listPos.x + listSize.x, listPos.y + listSize.y), Color(ImVec4(0.035f, 0.035f, 0.042f, 0.78f)), 2.0f * scale);

		ImGui::SetCursorScreenPos(ImVec2(listPos.x + 6.0f * scale, listPos.y + 6.0f * scale));
		ImGui::BeginChild("##friends_list", ImVec2(listSize.x - 12.0f * scale, listSize.y - 12.0f * scale), false, ImGuiWindowFlags_NoBackground);

		const std::vector<std::string> players = ServerPlayerNames();
		ImFont* labelFont = Menu::font16 ? Menu::font16 : ImGui::GetFont();
		ImGui::GetWindowDrawList()->AddText(labelFont, 16.0f * scale, ImGui::GetCursorScreenPos(), c.mutedText, "Server");
		ImGui::Dummy(ImVec2(1.0f, 24.0f * scale));

		int rowIndex = 0;
		if (players.empty())
		{
			ImGui::GetWindowDrawList()->AddText(labelFont, 16.0f * scale, ImGui::GetCursorScreenPos(), c.dimText, "No players found");
			ImGui::Dummy(ImVec2(1.0f, 30.0f * scale));
		}
		else
		{
			for (const std::string& player : players)
				RenderFriendRow(player, rowIndex++, true, scale);
		}

		if (!settings::friends.empty())
		{
			ImGui::Dummy(ImVec2(1.0f, 6.0f * scale));
			ImGui::GetWindowDrawList()->AddText(labelFont, 16.0f * scale, ImGui::GetCursorScreenPos(), c.mutedText, "Saved");
			ImGui::Dummy(ImVec2(1.0f, 24.0f * scale));

			for (const std::string& friendName : settings::friends)
			{
				if (std::find(players.begin(), players.end(), friendName) != players.end())
					continue;
				RenderFriendRow(friendName, rowIndex++, false, scale);
			}
		}

		ImGui::EndChild();
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
	}
}

void Menu::RenderMenu()
{
	static std::vector<std::unique_ptr<ModuleBase>>& modules = ModuleManager::GetModules();
	UpdateAnimations(modules);

	const float scale = UiScale();
	const float panelWidth = kPanelWidth * scale;
	const float panelGap = kPanelGap * scale;
	const float totalWidth = panelWidth * 6.0f + panelGap * 5.0f;
	const ImVec2 display = ImGui::GetIO().DisplaySize;
	float x = std::max(12.0f * scale, (display.x - totalWidth) * 0.5f);
	const float top = std::max(16.0f * scale, kPanelTop * scale);
	const float maxPanelHeight = std::max(220.0f * scale, display.y - top - 20.0f * scale);

	const std::array<PanelSpec, 4> modulePanels = {
		PanelSpec{ "Combat", PanelIcon::Combat, { "Combat" } },
		PanelSpec{ "Misc", PanelIcon::Misc, { "Inventory", "Utility" } },
		PanelSpec{ "Movement", PanelIcon::Movement, { "Movement" } },
		PanelSpec{ "Visual", PanelIcon::Visual, { "Visual" } },
	};

	for (const PanelSpec& panel : modulePanels)
	{
		RenderModulePanel(panel, ImVec2(x, top), panelWidth, maxPanelHeight, modules, scale);
		x += panelWidth + panelGap;
	}

	RenderMenuPanel(ImVec2(x, top), panelWidth, scale);
	x += panelWidth + panelGap;
	RenderConfigPanel(ImVec2(x, top), panelWidth, scale);
	RenderBottomBar(scale);
	RenderFriendsWindow(scale);
}
