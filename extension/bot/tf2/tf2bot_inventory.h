#ifndef NAVBOT_TF2BOT_INVENTORY_INTERFACE_H_
#define NAVBOT_TF2BOT_INVENTORY_INTERFACE_H_

#include <bot/interfaces/inventory.h>
#include "tf2bot_weaponinfo.h"

class CTF2BotInventory : public IInventory
{
public:
	CTF2BotInventory(CTF2Bot* bot);
	~CTF2BotInventory();

	std::shared_ptr<CTF2BotWeapon> GetActiveTFWeapon();

protected:
	CBotWeapon* CreateBotWeapon(CBaseEntity* weapon) override { return new CTF2BotWeapon(weapon); }
};

#endif // !NAVBOT_TF2BOT_INVENTORY_INTERFACE_H_
