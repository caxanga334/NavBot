#include <extension.h>
#include "dodsbot.h"
#include "dodsbot_inventory.h"

CDoDSBotInventory::CDoDSBotInventory(CBaseBot* bot) :
	IInventory(bot)
{
}

CDoDSBotInventory::~CDoDSBotInventory()
{
}

bool CDoDSBotInventory::HasBomb() const
{
	bool found = false;

	this->ForEveryWeapon([&found](const CBotWeapon* weapon) {
		if (strcmp(weapon->GetClassname().c_str(), "weapon_basebomb") == 0)
		{
			found = true;
		}
	});

	return found;
}
