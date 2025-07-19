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
	// Returns a weapon interface pointer if the bot currently owns the eureka effect
	const CTF2BotWeapon* GetTheEurekaEffect() const { return static_cast<const CTF2BotWeapon*>(IInventory::FindWeaponByEconIndex(589)); } /* 589 is the eureka effects econ index */
	// Returns a weapon capable of removing sappers or NULL if the bot doesn't own any
	const CTF2BotWeapon* GetSapperRemovalWeapon() const
	{
		const CTF2BotWeapon* weapon = static_cast<const CTF2BotWeapon*>(IInventory::FindWeaponByClassname("tf_weapon_wrench"));

		if (!weapon)
		{
			weapon = static_cast<const CTF2BotWeapon*>(IInventory::FindWeaponByTag("canremovesappers"));
		}

		return weapon;
	}

protected:
	CBotWeapon* CreateBotWeapon(CBaseEntity* weapon) override { return new CTF2BotWeapon(weapon); }

private:
};

#endif // !NAVBOT_TF2BOT_INVENTORY_INTERFACE_H_
