#include "bridgeAssist.h"

#include "menu/menu.h"
#include "util/logger.h"
#include "util/keys.h"
#include "moduleManager/commonData.h"

void BridgeAssist::Update() // Thanks to Steve987321 @ https://github.com/Steve987321/toadclient for the idea / implementation.
{
	bool isSneaking = Keys::IsKeyPressed(VK_SHIFT);

	if (!settings::BA_Enabled || !CommonData::SanityCheck() || SDK::minecraft->IsInGuiState())
	{
		if (!m_hasPressedShift && isSneaking && !settings::BA_OnlyOnShift)
		{
			UnSneak();
		}

		m_hasPressedShift = false;
		m_isBridging = false;
		return;
	}

	if (settings::BA_IgnoreForwardsMovement && Keys::IsKeyPressed(0x57))
	{
		if (!m_hasPressedShift && isSneaking && !settings::BA_OnlyOnShift)
		{
			UnSneak();
		}

		m_hasPressedShift = false;
		m_isBridging = false;
		return;
	}

	CItemStack item = SDK::minecraft->thePlayer->GetInventory().GetCurrentItem();
	if (item.GetInstance() == nullptr || item.GetItem().GetUnlocalizedName().find("tile") == std::string::npos)
	{
		bool found = false;
		if (m_isBridging && settings::BA_AutoSwap)
		{
			std::vector<CItemStack> hotbar = SDK::minecraft->thePlayer->GetInventory().GetMainInventory();
			for (int i = 0; i < 9; i++)
			{
				CItemStack stack = hotbar[i];
				if (stack.GetInstance() != nullptr && stack.GetItem().GetUnlocalizedName().find("tile") != std::string::npos)
				{
					SDK::minecraft->thePlayer->GetInventory().SetCurrentItemIndex(i);
					found = true;
					break;
				}
			}
		}

		if (!found)
		{
			if (!m_hasPressedShift && isSneaking && !settings::BA_OnlyOnShift)
			{
				UnSneak();
			}

			m_hasPressedShift = false;
			m_isBridging = false;
			return;
		}
	}

	if (settings::BA_OnlyOnShift)
	{
		if (!m_hasPressedShift && isSneaking)
		{
			m_hasPressedShift = true;
		}
	}

	if (CommonData::playerPitch < settings::BA_PitchCheck)
	{
		if (!m_hasPressedShift && isSneaking && !settings::BA_OnlyOnShift)
		{
			UnSneak();
		}

		m_hasPressedShift = false;
		m_isBridging = false;
		return;
	}

	if (settings::BA_OnlyOnShift && !m_hasPressedShift)
	{
		m_isBridging = false;
		return;
	}

	m_from = CommonData::playerPos;
	m_from.y += 1;
	Vector3 hitBlockPos;

	bool res = SDK::minecraft->theWorld->RayTraceBlocks(m_from, m_direction, hitBlockPos, false, false, false);

	// the vertical diff between the player and block under the player
	auto diffY = CommonData::playerPos.y - hitBlockPos.y;
	diffY -= 1;

	static bool jumped = false;
	if (GetAsyncKeyState(VK_SPACE) & 1)
	{
		jumped = true;
	}

	if (jumped)
	{
		UnSneak();

		// check if we are back on ground or going down
		if (SDK::minecraft->thePlayer->GetMotion().y < 0.0f || diffY <= 0.0f)
			jumped = false;
		m_isBridging = false;
		return;
	}

	if (diffY != 0 && diffY <= settings::BA_BlockCheck - 1)
	{
		UnSneak();
		m_isBridging = false;
		return;
	}


	m_isEdge = false;

	bool isFalling = std::abs(SDK::minecraft->thePlayer->GetMotion().y) > 0.5f;

	if (!isFalling)
	{
		m_isBridging = true;
		if (static_cast<int>(diffY) != 0 || res == false)
		{
			Sneak();
		}
		else
		{
			UnSneak();
		}
	}
	else
	{
		UnSneak();
	}
}

void BridgeAssist::RenderMenu()
{
	Menu::ToggleWithKeybind(&settings::BA_Enabled, settings::BA_Key);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
	Menu::HorizontalSeparator("Sep1");
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);

	Menu::Checkbox("Only on Shift", &settings::BA_OnlyOnShift);
	Menu::Checkbox("Ignore Forwards Movement", &settings::BA_IgnoreForwardsMovement);
	Menu::Checkbox("Auto Swap", &settings::BA_AutoSwap);
	Menu::Slider("Block Check", &settings::BA_BlockCheck, 1, 10);
	Menu::Slider("Pitch Check", &settings::BA_PitchCheck, 0.0f, 90.0f);
	Menu::Slider("Sneak Delay", &settings::BA_SneakDelay, 0, 300, ImVec2(0, 0), "%d ms");
	Menu::Slider("Unsneak Delay", &settings::BA_UnSneakDelay, 0, 300, ImVec2(0, 0), "%d ms");
}

void BridgeAssist::Sneak()
{
	m_unsneakQueued = false;
	if (m_prev)
	{
		m_isEdge = true;
		m_sneakQueued = false;
		return;
	}

	const auto now = std::chrono::steady_clock::now();
	if (!m_sneakQueued)
	{
		m_sneakRequestTime = now;
		m_sneakQueued = true;
	}

	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_sneakRequestTime).count();
	if (elapsed < settings::BA_SneakDelay)
		return;

	if (!settings::BA_OnlyOnShift) Keys::SendKey(VK_SHIFT, true);
	if (settings::BA_OnlyOnShift) Keys::SendKey(Menu::handleWindow, VK_SHIFT, true);
	m_prev = true;
	m_isEdge = true;
	m_sneakQueued = false;
}

void BridgeAssist::UnSneak()
{
	m_sneakQueued = false;
	m_isEdge = false;
	if (!m_prev)
	{
		m_unsneakQueued = false;
		return;
	}

	const auto now = std::chrono::steady_clock::now();
	if (!m_unsneakQueued)
	{
		m_unsneakRequestTime = now;
		m_unsneakQueued = true;
	}

	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_unsneakRequestTime).count();
	if (elapsed < settings::BA_UnSneakDelay)
		return;

	if (!settings::BA_OnlyOnShift) Keys::SendKey(VK_SHIFT, false);
	if (settings::BA_OnlyOnShift) Keys::SendKey(Menu::handleWindow, VK_SHIFT, false);
	m_prev = false;
	m_unsneakQueued = false;
}
