#ifndef __NAVBOT_DODSBOT_INVENTORY_INTERFACE_H_
#define __NAVBOT_DODSBOT_INVENTORY_INTERFACE_H_

#include <bot/interfaces/inventory.h>
#include "dodsbot_weaponinfo.h"

class CDoDSBotInventory : public IInventory
{
public:
	CDoDSBotInventory(CBaseBot* bot);
	~CDoDSBotInventory() override;

	const CDoDSBotWeapon* GetActiveDoDWeapon() const { return static_cast<const CDoDSBotWeapon*>(GetActiveBotWeapon()); }
	const CDoDSBotWeapon* GetMachineGun() const { 
		return static_cast<const CDoDSBotWeapon*>(IInventory::FindWeaponByTypeAndClass(WeaponInfo::WeaponType::COMBAT_WEAPON, WeaponInfo::WeaponClass::WEAPON_CLASS_MACHINEGUN)); 
	}
	const CDoDSBotWeapon* GetSniperRifle() const {
		return static_cast<const CDoDSBotWeapon*>(IInventory::FindWeaponByTypeAndClass(WeaponInfo::WeaponType::COMBAT_WEAPON, WeaponInfo::WeaponClass::WEAPON_CLASS_SNIPERRIFLE));
	}
	const CDoDSBotWeapon* GetRifleGrenade() const { return static_cast<const CDoDSBotWeapon*>(IInventory::FindWeaponByTag(TAG_RIFLE_GRENADE.data())); }

	bool HasBomb() const { return IInventory::FindWeaponByClassname(BOMB_CLASSNAME.data()) != nullptr; }
	bool HasMachineGun() const { return GetMachineGun() != nullptr; }
	bool HasSniperRifle() const { return GetSniperRifle() != nullptr; }
	bool HasRifleGrenade() const { return GetRifleGrenade() != nullptr; }

protected:
	CBotWeapon* CreateBotWeapon(CBaseEntity* weapon) override;

private:
	// weapon tags used for identifitication of some DoD:S weapons.
	static constexpr std::string_view BOMB_CLASSNAME = "weapon_basebomb";
	static constexpr std::string_view TAG_RIFLE_GRENADE = "riflegrenade";
};

#endif // !__NAVBOT_DODSBOT_INVENTORY_INTERFACE_H_
