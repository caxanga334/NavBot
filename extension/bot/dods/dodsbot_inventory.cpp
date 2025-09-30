#include NAVBOT_PCH_FILE
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
	return IInventory::FindWeaponByClassname("weapon_basebomb") != nullptr;
}
