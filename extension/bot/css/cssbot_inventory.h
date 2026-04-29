#ifndef __NAVBOT_CSS_BOT_INVENTORY_INTERFACE_H_
#define __NAVBOT_CSS_BOT_INVENTORY_INTERFACE_H_

#include "../interfaces/inventory.h"
#include "cssbot_weapon.h"

class CCSSBot;

class CCSSBotInventory : public IInventory
{
public:
	CCSSBotInventory(CCSSBot* bot);

	// Returns true if the bot has a sniper rifle.
	bool IsSniper() const { return IInventory::FindWeaponByTag("is_sniper") != nullptr; }
	// Returns true if the bot is carrying a C4.
	bool IsBombCarrier() const { return IInventory::FindWeaponByClassname("weapon_c4") != nullptr; }
	// Returns true if the bot has the default pistol.
	bool HasStartingPistol() const { return IInventory::FindWeaponByTag("is_starting_pistol") != nullptr; }

protected:
	CBotWeapon* CreateBotWeapon(CBaseEntity* weapon) override { return new CCSSBotWeapon(weapon); }

private:

};


#endif // !__NAVBOT_CSS_BOT_INVENTORY_INTERFACE_H_
