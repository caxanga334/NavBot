#include NAVBOT_PCH_FILE
#include "insmicbot.h"
#include "insmicbot_weapons.h"
#include "insmicbot_inventory.h"

CInsMICBotInventory::CInsMICBotInventory(CInsMICBot* bot) :
	IInventory(bot)
{
}

CBotWeapon* CInsMICBotInventory::CreateBotWeapon(CBaseEntity* weapon)
{
	return new CInsMICBotWeapon(weapon);
}
