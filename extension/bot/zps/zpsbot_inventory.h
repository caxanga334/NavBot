#ifndef __NAVBOT_ZPSBOT_INVENTORY_H_
#define __NAVBOT_ZPSBOT_INVENTORY_H_

#include <bot/interfaces/inventory.h>
#include "zpsbot_weaponinfo.h"

class CZPSBot;

class CZPSBotInventory : public IInventory
{
public:
	CZPSBotInventory(CZPSBot* bot);

	static constexpr int MAX_INVENTORY_SIZE = 5;

	void Reset() override;

	bool EquipWeapon(const CBotWeapon* weapon) const override;
	bool EquipWeapon(CBaseEntity* weapon) const override;
	void DropHeldWeapon() const override;
	// Returns the first weapon with low ammo.
	const CBotWeapon* GetWeaponWithLowAmmo();
	const CZPSBotWeapon* GetActiveZPSWeapon() const { return static_cast<const CZPSBotWeapon*>(GetActiveBotWeapon()); }
	int GetInventorySize() const
	{
		int size = 0;

		auto func = [&size](const CBotWeapon* base) {
			const CZPSBotWeapon* weapon = static_cast<const CZPSBotWeapon*>(base);
			size += weapon->GetZPSWeaponInfo()->GetInventorySize();
		};

		ForEveryWeapon(func);
		return size;
	}

protected:
	// Creates a new bot weapon object, override to use mod specific class
	CBotWeapon* CreateBotWeapon(CBaseEntity* weapon) { return new CZPSBotWeapon(weapon); }

private:
	const CBotWeapon* m_lastammosearchweapon;
};

#endif // !__NAVBOT_ZPSBOT_INVENTORY_H_
