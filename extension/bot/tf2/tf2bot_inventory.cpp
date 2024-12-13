#include <extension.h>
#include "tf2bot.h"
#include "tf2bot_inventory.h"

CTF2BotInventory::CTF2BotInventory(CTF2Bot* bot) :
	IInventory(bot)
{
}

CTF2BotInventory::~CTF2BotInventory()
{
}

std::shared_ptr<CTF2BotWeapon> CTF2BotInventory::GetActiveTFWeapon()
{
	return std::static_pointer_cast<CTF2BotWeapon>(IInventory::GetActiveBotWeapon());
}
