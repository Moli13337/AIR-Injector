#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <vector>
#include <string>
#include <functional>
#include <memory>

namespace Glassmorphism {

	// Animation states
	enum class AnimationState {
		None,
		SlidingIn,
		SlidingOut,
		FadingIn,
		FadingOut
	};

	// Animation data
	struct Animation {
		float progress = 0.0f;
		float duration = 0.3f; // 300ms default
		AnimationState state = AnimationState::None;
		ImVec2 startPos;
		ImVec2 endPos;
		float startAlpha = 0.0f;
		float endAlpha = 1.0f;
	};

	// Category frame data
	struct CategoryFrame {
		std::string name;
		ImVec2 position;
		ImVec2 size;
		bool isDragging = false;
		bool isDetached = false;
		bool isCollapsed = false;
		ImVec2 dragOffset;
		float headerRadius = 0.95f; // 95% curvature
	};

	// Module data
	struct ModuleItem {
		std::string name;
		bool enabled = false;
		bool isHovered = false;
		float glowIntensity = 0.0f;
	};

	// Toggle switch data
	struct ToggleSwitch {
		bool* value;
		ImVec2 size = ImVec2(44, 24);
		float knobRadius = 10.0f;
		float animationProgress = 0.0f;
	};

	// Custom slider data
	struct CustomSlider {
		float* value;
		float min = 0.0f;
		float max = 100.0f;
		ImVec2 size = ImVec2(200, 8);
		float knobRadius = 8.0f;
		bool isDragging = false;
	};

	// Glassmorphism style configuration
	struct StyleConfig {
		ImVec4 backgroundColor = ImVec4(0.1f, 0.1f, 0.1f, 0.7f); // Dark semi-transparent
		ImVec4 headerColor = ImVec4(0.15f, 0.15f, 0.15f, 0.8f);
		ImVec4 accentColor = ImVec4(0.0f, 0.5f, 1.0f, 1.0f); // Blue accent
		ImVec4 textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 disabledTextColor = ImVec4(1.0f, 1.0f, 1.0f, 0.6f);
		ImVec4 glowColor = ImVec4(0.0f, 0.5f, 1.0f, 0.3f);
		float blurAmount = 0.6f;
		float cornerRadius = 12.0f;
	};

	// Drawing functions
	void DrawRoundedRect(ImDrawList* drawList, ImVec2 min, ImVec2 max, ImU32 color, float radius, bool roundTopOnly = false);
	void DrawRoundedRectEx(ImDrawList* drawList, ImVec2 min, ImVec2 max, ImU32 color, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight);
	void DrawPillHeader(ImDrawList* drawList, ImVec2 min, ImVec2 max, ImU32 color, float curvature);
	void DrawGlassBackground(ImDrawList* drawList, ImVec2 min, ImVec2 max, const StyleConfig& style);
	void DrawGlowEffect(ImDrawList* drawList, ImVec2 center, float radius, ImU32 color, float intensity);

	// Component rendering
	bool RenderToggleSwitch(const char* label, bool* value, ImVec2 size = ImVec2(44, 24));
	bool RenderCustomSlider(const char* label, float* value, float min, float max, ImVec2 size = ImVec2(200, 8));
	bool RenderCategoryHeader(const char* label, ImVec2 size, bool* isDetached, bool* showSettings);
	bool RenderModuleItem(const char* label, bool enabled, bool isHovered, ImVec2 size);

	// Animation manager
	class AnimationManager {
	public:
		static void Update(float deltaTime);
		static void StartSlideAnimation(Animation& anim, ImVec2 from, ImVec2 to, float duration = 0.3f);
		static void StartFadeAnimation(Animation& anim, float fromAlpha, float toAlpha, float duration = 0.3f);
		static bool IsAnimating(const Animation& anim);
		static float GetProgress(const Animation& anim);
	};

	// Settings view
	class SettingsView {
	public:
		static void Render(const char* moduleName, ImVec2 centerPos, const StyleConfig& style);
		static void Open(const char* moduleName);
		static void Close();
		static bool IsOpen();
		static const char* GetCurrentModule();

	private:
		static inline bool isOpen = false;
		static inline std::string currentModule;
		static inline Animation slideAnimation;
	};

	// HUD renderer
	class HUDRenderer {
	public:
		static void Render(const std::vector<ModuleItem>& enabledModules, const StyleConfig& style);
		static bool ShouldRender(const std::vector<ModuleItem>& modules);
	};

	// Main GUI manager
	class GlassGUI {
	public:
		static void Init();
		static void Shutdown();
		static void Render();
		static void HandleInput();

		static void SetStyle(const StyleConfig& style);
		static const StyleConfig& GetStyle();

		static void AddCategory(const std::string& name);
		static void AddModule(const std::string& category, const std::string& moduleName, bool* enabled);

	private:
		static inline StyleConfig currentStyle;
		static inline std::vector<CategoryFrame> categories;
		static inline std::vector<std::pair<std::string, std::vector<ModuleItem>>> modulesByCategory;
		static inline bool isInitialized = false;
	};

} // namespace Glassmorphism
