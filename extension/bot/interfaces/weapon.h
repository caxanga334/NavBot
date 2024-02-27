#ifndef NAVBOT_WEAPON_STORAGE_INTERFACE_H_
#define NAVBOT_WEAPON_STORAGE_INTERFACE_H_
#pragma once

#include <basehandle.h>
#include "weaponinfo.h"
#include <entities/basecombatweapon.h>

struct edict_t;

class CBotWeapon
{
public:
	CBotWeapon(edict_t* entity);
	~CBotWeapon();

	// True if the weapon entity is valid
	bool IsValid() const;

	/**
	 * @brief Gets the Weapon info.
	 * @return Weapon Info pointer or NULL if no weapon info was obtained from the weapon info manager.
	 */
	inline const WeaponInfo* GetWeaponInfo() const { return m_info; }
	inline const entities::HBaseCombatWeapon& GetBaseCombatWeapon() const { return m_bcw; }
	inline int GetWeaponEconIndex() const { return m_econindex; }
	edict_t* GetEdict() const;
	CBaseEntity* GetEntity() const;
private:
	CBaseHandle m_handle;
	const WeaponInfo* m_info;
	entities::HBaseCombatWeapon m_bcw;
	int m_econindex;
};

#endif // !NAVBOT_WEAPON_STORAGE_INTERFACE_H_
