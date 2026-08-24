#include "sprint.h"

#include "moduleManager/commonData.h"
#include "menu/menu.h"
#include "util/keys.h"
#include "sdk/sdk.h"

void Sprint::Update()
{
    if (!settings::S_Enabled || !CommonData::SanityCheck() || SDK::minecraft->IsInGuiState() || Menu::open)
    {
        if (m_isHoldingCtrl)
        {
            Keys::SendKey(VK_CONTROL, false);
            m_isHoldingCtrl = false;
        }
        return;
    }

    // Check if player is moving (WASD keys)
    bool isMoving = Keys::IsKeyPressed(0x57) || Keys::IsKeyPressed(0x41) || Keys::IsKeyPressed(0x53) || Keys::IsKeyPressed(0x44);

    // Determine if we should sprint
    bool shouldSprint = isMoving || settings::S_WhenStill;

    if (shouldSprint && !m_isHoldingCtrl)
    {
        Keys::SendKey(VK_CONTROL, true);
        m_isHoldingCtrl = true;
    }
    else if (!shouldSprint && m_isHoldingCtrl)
    {
        Keys::SendKey(VK_CONTROL, false);
        m_isHoldingCtrl = false;
    }
}

void Sprint::RenderMenu()
{
    Menu::ToggleWithKeybind(&settings::S_Enabled, settings::S_Key);

    if (settings::S_Enabled)
    {
        Menu::Checkbox("In Water", &settings::S_InWater);
        Menu::Checkbox("In Air", &settings::S_InAir);
        Menu::Checkbox("When Still", &settings::S_WhenStill);
    }
}
