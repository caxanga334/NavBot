#ifndef __NAVBOT_ZPSBOT_INVENTORY_H_
#define __NAVBOT_ZPSBOT_INVENTORY_H_

#include <bot/interfaces/inventory.h>

class CZPSBot;

class CZPSBotInventory : public IInventory
{
public:
	CZPSBotInventory(CZPSBot* bot);

	void Reset() override;

	bool EquipWeapon(const CBotWeapon* weapon) const override;
	bool EquipWeapon(CBaseEntity* weapon) const override;
	// Returns the first weapon with low ammo.
	const CBotWeapon* GetWeaponWithLowAmmo();

private:
	const CBotWeapon* m_lastammosearchweapon;
};

#endif // !__NAVBOT_ZPSBOT_INVENTORY_H_
