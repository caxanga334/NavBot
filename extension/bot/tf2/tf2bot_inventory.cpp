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

bool CTF2BotInventory::IsAmmoLow(const bool heldOnly)
{
	CTF2Bot* me = GetBot<CTF2Bot>();

	if (me->GetMyClassType() == TeamFortress2::TFClassType::TFClass_Engineer)
	{
		if (me->GetAmmoOfIndex(static_cast<int>(TeamFortress2::TF_AMMO_METAL)) < TeamFortress2::TF_DEFAULT_MAX_METAL)
		{
			return true;
		}
	}

	return IInventory::IsAmmoLow(heldOnly);
}

