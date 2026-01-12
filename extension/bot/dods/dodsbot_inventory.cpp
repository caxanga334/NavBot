#include NAVBOT_PCH_FILE
#include <extension.h>
#include "dodsbot.h"
#include "dodsbot_weaponinfo.h"
#include "dodsbot_inventory.h"

CDoDSBotInventory::CDoDSBotInventory(CBaseBot* bot) :
	IInventory(bot)
{
}

CDoDSBotInventory::~CDoDSBotInventory()
{
}

CBotWeapon* CDoDSBotInventory::CreateBotWeapon(CBaseEntity* weapon)
{
	return new CDoDSBotWeapon(weapon);
}
