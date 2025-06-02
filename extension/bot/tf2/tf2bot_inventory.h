#ifndef NAVBOT_TF2BOT_INVENTORY_INTERFACE_H_
#define NAVBOT_TF2BOT_INVENTORY_INTERFACE_H_

#include <bot/interfaces/inventory.h>
#include "tf2bot_weaponinfo.h"

class CTF2BotInventory : public IInventory
{
public:
	CTF2BotInventory(CTF2Bot* bot);
	~CTF2BotInventory();

	bool IsAmmoLow(const bool heldOnly = true) override;

	const CTF2BotWeapon* GetActiveTFWeapon() const { return static_cast<const CTF2BotWeapon*>(IInventory::GetActiveBotWeapon()); }

protected:
	CBotWeapon* CreateBotWeapon(CBaseEntity* weapon) override { return new CTF2BotWeapon(weapon); }

private:
};

#endif // !NAVBOT_TF2BOT_INVENTORY_INTERFACE_H_
