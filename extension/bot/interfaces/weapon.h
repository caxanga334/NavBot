#ifndef NAVBOT_WEAPON_STORAGE_INTERFACE_H_
#define NAVBOT_WEAPON_STORAGE_INTERFACE_H_
#pragma once

#include <string>
#include <sdkports/sdk_ehandle.h>
#include "weaponinfo.h"
#include <entities/basecombatweapon.h>

class CBotWeapon
{
public:
	CBotWeapon(CBaseEntity* entity);
	virtual ~CBotWeapon();

	// True if the weapon entity is valid
	bool IsValid() const;

	/**
	 * @brief Gets the Weapon info.
	 * @return Weapon Info pointer or NULL if no weapon info was obtained from the weapon info manager.
	 */
	inline const std::shared_ptr<WeaponInfo>& GetWeaponInfo() const { return m_info; }
	inline const entities::HBaseCombatWeapon& GetBaseCombatWeapon() const { return m_bcw; }
	inline int GetWeaponEconIndex() const { return m_econindex; }
	edict_t* GetEdict() const;
	CBaseEntity* GetEntity() const;
	int GetIndex() const;

	const std::string& GetClassname() const { return m_classname; }

protected:
	std::shared_ptr<WeaponInfo> m_info;

private:
	CHandle<CBaseEntity> m_handle;
	entities::HBaseCombatWeapon m_bcw;
	int m_econindex;
	std::string m_classname;
};

#endif // !NAVBOT_WEAPON_STORAGE_INTERFACE_H_
