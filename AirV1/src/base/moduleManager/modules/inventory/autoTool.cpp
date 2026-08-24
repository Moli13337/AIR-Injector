#include "autoTool.h"
#include "moduleManager/commonData.h"
#include "menu/menu.h"
#include "sdk/sdk.h"
#include "configManager/settings.h"
#include <vector>

void AutoTool::Update()
{
	if (!settings::AT_Enabled || !CommonData::SanityCheck() || SDK::minecraft->IsInGuiState() || Menu::open)
		return;

	// Get the block the player is looking at
	auto player = SDK::minecraft->GetPlayer();
	if (!player) return;

	auto objectMouseOver = player->GetObjectMouseOver();
	if (!objectMouseOver || objectMouseOver->GetType() != "Block")
		return;

	auto blockPos = objectMouseOver->GetBlockPos();
	auto block = SDK::minecraft->GetWorld()->GetBlock(blockPos);
	if (!block) return;

	// Get block material to determine best tool
	std::string material = block->GetMaterial();
	int bestToolSlot = -1;
	float bestSpeed = 0.0f;

	// Check hotbar for best tool
	for (int i = 0; i < 9; i++) {
		auto stack = player->GetInventory()->GetStackInSlot(i);
		if (!stack || !stack->IsValid()) continue;

		auto item = stack->GetItem();
		if (!item) continue;

		// Check if this tool is effective for the block
		float toolSpeed = GetToolSpeed(item, material, block);
		
		// Apply enchantment preferences
		if (settings::AT_PreferFortune && HasEnchantment(stack, "fortune")) {
			toolSpeed *= 1.2f; // Prefer fortune tools
		}
		if (settings::AT_PreferSilkTouch && HasEnchantment(stack, "silk_touch")) {
			toolSpeed *= 1.1f; // Prefer silk touch tools
		}

		if (toolSpeed > bestSpeed) {
			bestSpeed = toolSpeed;
			bestToolSlot = i;
		}
	}

	// Switch to best tool if found
	if (bestToolSlot != -1 && bestToolSlot != player->GetInventory()->GetCurrentSlot()) {
		player->GetInventory()->SetCurrentSlot(bestToolSlot);
	}
}

float AutoTool::GetToolSpeed(Item* item, const std::string& material, Block* block)
{
	// Simplified tool speed calculation
	std::string toolType = item->GetToolType();
	
	// Wood blocks - axe is best
	if (material == "wood" || material == "log") {
		return toolType == "axe" ? 6.0f : 1.0f;
	}
	
	// Stone blocks - pickaxe is best
	if (material == "stone" || material == "rock") {
		return toolType == "pickaxe" ? 6.0f : 1.0f;
	}
	
	// Dirt/sand blocks - shovel is best
	if (material == "dirt" || material == "sand" || material == "gravel") {
		return toolType == "shovel" ? 6.0f : 1.0f;
	}
	
	// Plants - hoe or shears
	if (material == "plant" || material == "leaves") {
		return toolType == "hoe" ? 6.0f : (toolType == "shears" ? 8.0f : 1.0f);
	}
	
	return 1.0f; // Default speed
}

bool AutoTool::HasEnchantment(ItemStack* stack, const std::string& enchantment)
{
	// Placeholder for enchantment check
	// TODO: Implement proper enchantment checking when SDK methods are available
	return false;
}

void AutoTool::RenderMenu()
{
	Menu::ToggleWithKeybind(&settings::AT_Enabled, settings::AT_Key);

	if (settings::AT_Enabled)
	{
		Menu::Checkbox("Prefer Fortune", &settings::AT_PreferFortune);
		Menu::Checkbox("Prefer Silk Touch", &settings::AT_PreferSilkTouch);
	}
}
