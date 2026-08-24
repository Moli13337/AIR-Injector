#include "velocity.h"

#include <chrono>

#include "menu/menu.h"
#include "moduleManager/commonData.h"
#include "util/keys.h"
#include "util/math/random.h"

void Velocity::Update()
{
	static int lastingTime = 0;
	static bool pendingJump = false;
	static std::chrono::time_point<std::chrono::steady_clock> hitTime;

	if (lastingTime == 1)
	{
		Keys::SendKey(VK_SPACE, false);
		lastingTime = 0;
	}
	else if (lastingTime > 1)
	{
		lastingTime--;
	}

	if (!settings::Velocity_Enabled || !CommonData::SanityCheck() || SDK::minecraft->IsInGuiState())
	{
		pendingJump = false;
		return;
	}

	if (pendingJump)
	{
		const auto now = std::chrono::steady_clock::now();
		const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - hitTime).count();
		if (elapsed >= static_cast<long long>(settings::Velocity_JRReactionTime))
		{
			Keys::SendKey(VK_SPACE);
			lastingTime = Random::Int(40, 70);
			pendingJump = false;
		}
		return;
	}

	int hurtResistantTime = SDK::minecraft->thePlayer->GetHurtResistantTime();

	static bool canBeHit = true;
	if (!canBeHit && hurtResistantTime == 0)
	{
		canBeHit = true;
	}

	if (!canBeHit)
	{
		return;
	}

	if (hurtResistantTime <= 10)
	{
		return;
	}

	if (settings::Velocity_Mode == 0)
	{
		if (Random::Int(0, 100) <= settings::Velocity_JRChange)
		{
			hitTime = std::chrono::steady_clock::now();
			pendingJump = true;
			canBeHit = false;
		}
	}
}

void Velocity::RenderMenu()
{
	Menu::ToggleWithKeybind(&settings::Velocity_Enabled, settings::Velocity_Key);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
	Menu::HorizontalSeparator("Sep1");
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);

	Menu::Dropdown("Mode", settings::Velocity_ModeList, &settings::Velocity_Mode, 1);
	if (settings::Velocity_Mode == 0)
	{
		Menu::Slider("Reaction Time", &settings::Velocity_JRReactionTime, 0.f, 1000.f, ImVec2(0,0), "%.2f ms");
		Menu::Slider("Chance", &settings::Velocity_JRChange, 0, 100, ImVec2(0, 0), "%d%%");
	}
}
