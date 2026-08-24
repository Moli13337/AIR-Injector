#include "glassmorphism.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <algorithm>
#include <cmath>

namespace Glassmorphism {

	// ============================================
	// DRAWING FUNCTIONS
	// ============================================

	void DrawRoundedRect(ImDrawList* drawList, ImVec2 min, ImVec2 max, ImU32 color, float radius, bool roundTopOnly) {
		if (roundTopOnly) {
			drawList->AddRectFilled(min, max, color, radius, ImDrawFlags_RoundCornersTop);
		} else {
			drawList->AddRectFilled(min, max, color, radius, ImDrawFlags_RoundCornersAll);
		}
	}

	void DrawRoundedRectEx(ImDrawList* drawList, ImVec2 min, ImVec2 max, ImU32 color, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight) {
		float w = max.x - min.x;
		float h = max.y - min.y;

		// Draw rounded corners using arc segments
		const int segments = 20;
		const float step = IM_PI / 2.0f / segments;

		drawList->PathClear();

		// Top-left corner
		if (radiusTopLeft > 0) {
			for (int i = 0; i <= segments; i++) {
				float a = IM_PI * 1.5f + i * step;
				float x = min.x + radiusTopLeft * (1 - cosf(a));
				float y = min.y + radiusTopLeft * (1 - sinf(a));
				drawList->PathLineTo(ImVec2(x, y));
			}
		} else {
			drawList->PathLineTo(min);
		}

		// Top-right corner
		if (radiusTopRight > 0) {
			for (int i = 0; i <= segments; i++) {
				float a = IM_PI + i * step;
				float x = max.x - radiusTopRight * (1 - cosf(a));
				float y = min.y + radiusTopRight * (1 - sinf(a));
				drawList->PathLineTo(ImVec2(x, y));
			}
		} else {
			drawList->PathLineTo(ImVec2(max.x, min.y));
		}

		// Bottom-right corner
		if (radiusBottomRight > 0) {
			for (int i = 0; i <= segments; i++) {
				float a = IM_PI * 0.5f + i * step;
				float x = max.x - radiusBottomRight * (1 - cosf(a));
				float y = max.y - radiusBottomRight * (1 - sinf(a));
				drawList->PathLineTo(ImVec2(x, y));
			}
		} else {
			drawList->PathLineTo(max);
		}

		// Bottom-left corner
		if (radiusBottomLeft > 0) {
			for (int i = 0; i <= segments; i++) {
				float a = i * step;
				float x = min.x + radiusBottomLeft * (1 - cosf(a));
				float y = max.y - radiusBottomLeft * (1 - sinf(a));
				drawList->PathLineTo(ImVec2(x, y));
			}
		} else {
			drawList->PathLineTo(ImVec2(min.x, max.y));
		}

		drawList->PathFillConvex(color);
	}

	void DrawPillHeader(ImDrawList* drawList, ImVec2 min, ImVec2 max, ImU32 color, float curvature) {
		float height = max.y - min.y;
		float radius = height * curvature * 0.5f;
		
		// Ensure radius doesn't exceed half the height
		radius = std::min(radius, height * 0.5f);
		
		DrawRoundedRectEx(drawList, min, max, color, radius, radius, 0, 0);
	}

	void DrawGlassBackground(ImDrawList* drawList, ImVec2 min, ImVec2 max, const StyleConfig& style) {
		ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(style.backgroundColor);
		ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
			style.headerColor.x * 1.2f,
			style.headerColor.y * 1.2f,
			style.headerColor.z * 1.2f,
			style.headerColor.w
		));

		// Draw main background with rounded corners
		DrawRoundedRect(drawList, min, max, bgColor, style.cornerRadius);

		// Draw subtle border
		drawList->AddRect(min, max, borderColor, style.cornerRadius, ImDrawFlags_RoundCornersAll, 1.0f);

		// Draw subtle gradient overlay for glass effect
		ImU32 gradientTop = ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 0.05f));
		ImU32 gradientBottom = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.05f));
		
		ImVec2 gradMin = min;
		ImVec2 gradMax = ImVec2(max.x, min.y + (max.y - min.y) * 0.3f);
		drawList->AddRectFilledMultiColor(gradMin, gradMax, gradientTop, gradientTop, gradientBottom, gradientBottom);
	}

	void DrawGlowEffect(ImDrawList* drawList, ImVec2 center, float radius, ImU32 color, float intensity) {
		if (intensity <= 0.0f) return;

		const int segments = 32;
		const float step = IM_PI * 2.0f / segments;

		for (int i = 0; i < segments; i++) {
			float a1 = i * step;
			float a2 = (i + 1) * step;
			
			ImVec2 p1 = ImVec2(center.x + cosf(a1) * radius, center.y + sinf(a1) * radius);
			ImVec2 p2 = ImVec2(center.x + cosf(a2) * radius, center.y + sinf(a2) * radius);
			
			ImU32 alphaColor = color;
			// Modify alpha based on intensity
			ImColor c(color);
			c.Value.w *= intensity;
			alphaColor = c;

			drawList->AddLine(p1, p2, alphaColor, 2.0f);
		}
	}

	// ============================================
	// COMPONENT RENDERING
	// ============================================

	bool RenderCustomToggle(const char* label, bool* value, ImVec2 size) {
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems) return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

		const ImRect frame_bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
		const ImRect total_bb(frame_bb.Min, ImVec2(frame_bb.Max.x + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), frame_bb.Max.y));

		ImGui::ItemSize(total_bb, style.FramePadding.y);
		if (!ImGui::ItemAdd(total_bb, id, &frame_bb)) return false;

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(frame_bb, id, &hovered, &held);

		if (pressed) {
			*value = !*value;
		}

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 center = frame_bb.GetCenter();
		float radius = size.y * 0.5f;

		// Background pill
		ImU32 bgColor = *value ? ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.5f, 1.0f, 1.0f)) 
		                     : ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
		DrawRoundedRect(drawList, frame_bb.Min, frame_bb.Max, bgColor, radius);

		// Knob position
		float knobX = *value ? frame_bb.Max.x - radius - 2.0f : frame_bb.Min.x + radius + 2.0f;
		ImVec2 knobPos = ImVec2(knobX, center.y);

		// Draw knob
		ImU32 knobColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		drawList->AddCircleFilled(knobPos, radius - 2.0f, knobColor, 32);

		// Draw shadow/glow
		if (*value) {
			ImU32 glowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.5f, 1.0f, 0.3f));
			DrawGlowEffect(drawList, knobPos, radius + 4.0f, glowColor, 0.5f);
		}

		// Label
		if (label_size.x > 0.0f) {
			ImGui::RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);
		}

		return pressed;
	}

	bool RenderCustomSlider(const char* label, float* value, float min, float max, ImVec2 size) {
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems) return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

		const ImRect frame_bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
		const ImRect total_bb(frame_bb.Min, ImVec2(frame_bb.Max.x + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), frame_bb.Max.y));

		ImGui::ItemSize(total_bb, style.FramePadding.y);
		if (!ImGui::ItemAdd(total_bb, id, &frame_bb)) return false;

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(frame_bb, id, &hovered, &held);

		if (held) {
			float ratio = (ImGui::GetMousePos().x - frame_bb.Min.x) / (frame_bb.Max.x - frame_bb.Min.x);
			*value = min + ratio * (max - min);
			*value = std::clamp(*value, min, max);
		}

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		float ratio = (*value - min) / (max - min);
		ratio = std::clamp(ratio, 0.0f, 1.0f);

		// Background line
		ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImVec2 lineMin = frame_bb.Min;
		ImVec2 lineMax = ImVec2(frame_bb.Max.x, frame_bb.Min.y + size.y * 0.5f);
		drawList->AddRectFilled(lineMin, ImVec2(frame_bb.Max.x, lineMin.y + size.y), bgColor, size.y * 0.5f);

		// Filled line
		ImU32 fillColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.5f, 1.0f, 1.0f));
		ImVec2 fillMax = ImVec2(frame_bb.Min.x + (frame_bb.Max.x - frame_bb.Min.x) * ratio, lineMin.y + size.y);
		drawList->AddRectFilled(lineMin, fillMax, fillColor, size.y * 0.5f);

		// Knob
		float knobX = frame_bb.Min.x + (frame_bb.Max.x - frame_bb.Min.x) * ratio;
		ImVec2 knobPos = ImVec2(knobX, frame_bb.Min.y + size.y * 0.5f);
		float knobRadius = size.y * 0.8f;

		ImU32 knobColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		drawList->AddCircleFilled(knobPos, knobRadius, knobColor, 32);

		// Knob glow
		ImU32 glowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.5f, 1.0f, 0.3f));
		DrawGlowEffect(drawList, knobPos, knobRadius + 3.0f, glowColor, held ? 0.8f : 0.4f);

		// Label
		if (label_size.x > 0.0f) {
			ImGui::RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);
		}

		return pressed;
	}

	bool RenderCategoryHeader(const char* label, ImVec2 size, bool* isDetached, bool* showSettings) {
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems) return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);

		const ImRect frame_bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
		ImGui::ItemSize(size);

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(frame_bb, id, &hovered, &held);

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// Draw pill header
		ImU32 headerColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.15f, 0.15f, 0.15f, 0.9f));
		DrawPillHeader(drawList, frame_bb.Min, frame_bb.Max, headerColor, 0.95f);

		// Draw category name
		ImVec2 textSize = ImGui::CalcTextSize(label);
		ImVec2 textPos = ImVec2(
			frame_bb.Min.x + 15.0f,
			frame_bb.Min.y + (size.y - textSize.y) * 0.5f
		);
		drawList->AddText(textPos, ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1)), label);

		// Draw detach icon (square with arrow)
		ImVec2 detachIconPos = ImVec2(frame_bb.Max.x - 50.0f, frame_bb.Min.y + (size.y - 16.0f) * 0.5f);
		ImU32 iconColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
		drawList->AddRect(detachIconPos, ImVec2(detachIconPos.x + 16.0f, detachIconPos.y + 16.0f), iconColor, 2.0f);
		// Arrow
		drawList->AddLine(ImVec2(detachIconPos.x + 8.0f, detachIconPos.y - 2.0f), ImVec2(detachIconPos.x + 8.0f, detachIconPos.y + 6.0f), iconColor, 2.0f);
		drawList->AddLine(ImVec2(detachIconPos.x + 8.0f, detachIconPos.y + 6.0f), ImVec2(detachIconPos.x + 14.0f, detachIconPos.y + 6.0f), iconColor, 2.0f);

		// Draw settings icon (gear)
		ImVec2 settingsIconPos = ImVec2(frame_bb.Max.x - 25.0f, frame_bb.Min.y + (size.y - 16.0f) * 0.5f);
		drawList->AddCircle(ImVec2(settingsIconPos.x + 8.0f, settingsIconPos.y + 8.0f), 6.0f, iconColor, 0, 2.0f);
		// Gear teeth
		for (int i = 0; i < 6; i++) {
			float angle = i * IM_PI / 3.0f;
			ImVec2 toothStart = ImVec2(
				settingsIconPos.x + 8.0f + cosf(angle) * 6.0f,
				settingsIconPos.y + 8.0f + sinf(angle) * 6.0f
			);
			ImVec2 toothEnd = ImVec2(
				settingsIconPos.x + 8.0f + cosf(angle) * 9.0f,
				settingsIconPos.y + 8.0f + sinf(angle) * 9.0f
			);
			drawList->AddLine(toothStart, toothEnd, iconColor, 2.0f);
		}

		// Handle detach click
		if (pressed && ImGui::GetMousePos().x > detachIconPos.x && ImGui::GetMousePos().x < detachIconPos.x + 16.0f) {
			*isDetached = !*isDetached;
		}

		// Handle settings click
		if (pressed && ImGui::GetMousePos().x > settingsIconPos.x && ImGui::GetMousePos().x < settingsIconPos.x + 16.0f) {
			*showSettings = true;
		}

		return pressed;
	}

	bool RenderModuleItem(const char* label, bool enabled, bool isHovered, ImVec2 size) {
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems) return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);

		const ImRect frame_bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
		ImGui::ItemSize(size);

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(frame_bb, id, &hovered, &held);

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// Background
		ImU32 bgColor = isHovered ? ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.2f, 0.2f, 0.5f))
		                         : ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		drawList->AddRectFilled(frame_bb.Min, frame_bb.Max, bgColor, 4.0f);

		// Text
		ImVec2 textSize = ImGui::CalcTextSize(label);
		ImVec2 textPos = ImVec2(
			frame_bb.Min.x + 10.0f,
			frame_bb.Min.y + (size.y - textSize.y) * 0.5f
		);

		ImU32 textColor = enabled ? ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1))
		                        : ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 0.6f));

		if (enabled) {
			// Bold weight with glow
			drawList->AddText(textPos, textColor, label);
			ImU32 glowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.5f, 1.0f, 0.3f));
			DrawGlowEffect(drawList, ImVec2(textPos.x + textSize.x * 0.5f, textPos.y + textSize.y * 0.5f), 
			               textSize.x * 0.6f, glowColor, 0.5f);
		} else {
			drawList->AddText(textPos, textColor, label);
		}

		return pressed;
	}

	// ============================================
	// ANIMATION MANAGER
	// ============================================

	void AnimationManager::Update(float deltaTime) {
		// This would be called each frame to update active animations
		// For now, this is a placeholder
	}

	void AnimationManager::StartSlideAnimation(Animation& anim, ImVec2 from, ImVec2 to, float duration) {
		anim.startPos = from;
		anim.endPos = to;
		anim.duration = duration;
		anim.progress = 0.0f;
		anim.state = AnimationState::SlidingIn;
	}

	void AnimationManager::StartFadeAnimation(Animation& anim, float fromAlpha, float toAlpha, float duration) {
		anim.startAlpha = fromAlpha;
		anim.endAlpha = toAlpha;
		anim.duration = duration;
		anim.progress = 0.0f;
		anim.state = AnimationState::FadingIn;
	}

	bool AnimationManager::IsAnimating(const Animation& anim) {
		return anim.state != AnimationState::None && anim.progress < 1.0f;
	}

	float AnimationManager::GetProgress(const Animation& anim) {
		return anim.progress;
	}

	// ============================================
	// SETTINGS VIEW
	// ============================================

	void SettingsView::Render(const char* moduleName, ImVec2 centerPos, const StyleConfig& style) {
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 size = ImVec2(400, 300);
		ImVec2 min = ImVec2(centerPos.x - size.x * 0.5f, centerPos.y - size.y * 0.5f);
		ImVec2 max = ImVec2(centerPos.x + size.x * 0.5f, centerPos.y + size.y * 0.5f);

		// Draw glassmorphism background
		DrawGlassBackground(drawList, min, max, style);

		// Draw title
		ImVec2 titleSize = ImGui::CalcTextSize(moduleName);
		ImVec2 titlePos = ImVec2(
			centerPos.x - titleSize.x * 0.5f,
			min.y + 20.0f
		);
		drawList->AddText(titlePos, ImGui::ColorConvertFloat4ToU32(style.textColor), moduleName);

		// Draw settings content placeholder
		ImVec2 contentPos = ImVec2(min.x + 20.0f, min.y + 60.0f);
		drawList->AddText(contentPos, ImGui::ColorConvertFloat4ToU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)), "Settings will be rendered here");
	}

	void SettingsView::Open(const char* moduleName) {
		isOpen = true;
		currentModule = moduleName;
	}

	void SettingsView::Close() {
		isOpen = false;
		currentModule = "";
	}

	bool SettingsView::IsOpen() {
		return isOpen;
	}

	const char* SettingsView::GetCurrentModule() {
		return currentModule.c_str();
	}

	// ============================================
	// HUD RENDERER
	// ============================================

	void HUDRenderer::Render(const std::vector<ModuleItem>& enabledModules, const StyleConfig& style) {
		if (enabledModules.empty()) return;

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 startPos = ImVec2(10.0f, 10.0f);
		float itemHeight = 20.0f;
		float spacing = 5.0f;

		for (size_t i = 0; i < enabledModules.size(); i++) {
			ImVec2 itemPos = ImVec2(startPos.x, startPos.y + i * (itemHeight + spacing));
			drawList->AddText(itemPos, ImGui::ColorConvertFloat4ToU32(style.textColor), enabledModules[i].name.c_str());
		}
	}

	bool HUDRenderer::ShouldRender(const std::vector<ModuleItem>& modules) {
		for (const auto& module : modules) {
			if (module.enabled) return true;
		}
		return false;
	}

	// ============================================
	// MAIN GUI MANAGER
	// ============================================

	void GlassGUI::Init() {
		if (isInitialized) return;
		isInitialized = true;
	}

	void GlassGUI::Shutdown() {
		categories.clear();
		modulesByCategory.clear();
		isInitialized = false;
	}

	void GlassGUI::Render() {
		if (!isInitialized) return;

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 screenPos = ImGui::GetCursorScreenPos();

		// Render category frames
		for (auto& category : categories) {
			ImVec2 frameMin = ImVec2(screenPos.x + category.position.x, screenPos.y + category.position.y);
			ImVec2 frameMax = ImVec2(frameMin.x + category.size.x, frameMin.y + category.size.y);

			// Draw glass background
			DrawGlassBackground(drawList, frameMin, frameMax, currentStyle);

			// Draw pill header
			ImVec2 headerMax = ImVec2(frameMax.x, frameMin.y + 40.0f);
			ImU32 headerColor = ImGui::ColorConvertFloat4ToU32(currentStyle.headerColor);
			DrawPillHeader(drawList, frameMin, headerMax, headerColor, category.headerRadius);

			// Draw category name
			ImVec2 textSize = ImGui::CalcTextSize(category.name.c_str());
			ImVec2 textPos = ImVec2(
				frameMin.x + 15.0f,
				frameMin.y + (40.0f - textSize.y) * 0.5f
			);
			drawList->AddText(textPos, ImGui::ColorConvertFloat4ToU32(currentStyle.textColor), category.name.c_str());

			// Draw detach and settings icons
			ImVec2 detachPos = ImVec2(headerMax.x - 50.0f, frameMin.y + 12.0f);
			ImVec2 settingsPos = ImVec2(headerMax.x - 25.0f, frameMin.y + 12.0f);
			ImU32 iconColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

			// Detach icon
			drawList->AddRect(detachPos, ImVec2(detachPos.x + 16.0f, detachPos.y + 16.0f), iconColor, 2.0f);
			drawList->AddLine(ImVec2(detachPos.x + 8.0f, detachPos.y + 2.0f), ImVec2(detachPos.x + 8.0f, detachPos.y + 10.0f), iconColor, 2.0f);
			drawList->AddLine(ImVec2(detachPos.x + 8.0f, detachPos.y + 10.0f), ImVec2(detachPos.x + 14.0f, detachPos.y + 10.0f), iconColor, 2.0f);

			// Settings icon
			drawList->AddCircle(ImVec2(settingsPos.x + 8.0f, settingsPos.y + 8.0f), 6.0f, iconColor, 0, 2.0f);
		}

		// Render settings view if open
		if (SettingsView::IsOpen()) {
			ImVec2 center = ImVec2(screenPos.x + 500.0f, screenPos.y + 325.0f);
			SettingsView::Render(SettingsView::GetCurrentModule(), center, currentStyle);
		}
	}

	void GlassGUI::HandleInput() {
		// Handle dragging, clicking, etc.
	}

	void GlassGUI::SetStyle(const StyleConfig& style) {
		currentStyle = style;
	}

	const StyleConfig& GlassGUI::GetStyle() {
		return currentStyle;
	}

	void GlassGUI::AddCategory(const std::string& name) {
		CategoryFrame frame;
		frame.name = name;
		frame.position = ImVec2(50.0f + categories.size() * 220.0f, 50.0f);
		frame.size = ImVec2(200.0f, 400.0f);
		frame.headerRadius = 0.95f;
		categories.push_back(frame);
	}

	void GlassGUI::AddModule(const std::string& category, const std::string& moduleName, bool* enabled) {
		for (auto& cat : modulesByCategory) {
			if (cat.first == category) {
				ModuleItem item;
				item.name = moduleName;
				item.enabled = *enabled;
				cat.second.push_back(item);
				return;
			}
		}
		// Category doesn't exist, create it
		modulesByCategory.push_back({category, {ModuleItem{moduleName, *enabled}}});
	}

} // namespace Glassmorphism
