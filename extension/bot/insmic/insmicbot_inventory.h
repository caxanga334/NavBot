#ifndef __NAVBOT_INSMICBOT_INVENTORY_INTERFACE_H_
#define __NAVBOT_INSMICBOT_INVENTORY_INTERFACE_H_

#include <bot/interfaces/inventory.h>

class CInsMICBot;

class CInsMICBotInventory : public IInventory
{
public:
	CInsMICBotInventory(CInsMICBot* bot);

	void BuildInventory() override;
	bool EquipWeapon(const CBotWeapon* weapon) const override;
	bool EquipWeapon(CBaseEntity* weapon) const override;

protected:
	CBotWeapon* CreateBotWeapon(CBaseEntity* weapon) override;

private:

};


#endif // !__NAVBOT_INSMICBOT_INVENTORY_INTERFACE_H_