#ifndef __NAVBOT_ZPSBOT_INVENTORY_H_
#define __NAVBOT_ZPSBOT_INVENTORY_H_

#include <bot/interfaces/inventory.h>

class CZPSBot;

class CZPSBotInventory : public IInventory
{
public:
	CZPSBotInventory(CZPSBot* bot);

	bool EquipWeapon(const CBotWeapon* weapon) const override;
	bool EquipWeapon(CBaseEntity* weapon) const override;

private:

};

#endif // !__NAVBOT_ZPSBOT_INVENTORY_H_
