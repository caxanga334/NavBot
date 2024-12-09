#ifndef NAVBOT_WEAPON_STORAGE_INTERFACE_H_
#define NAVBOT_WEAPON_STORAGE_INTERFACE_H_
#pragma once

#include <sdkports/sdk_ehandle.h>
#include "weaponinfo.h"
#include <entities/basecombatweapon.h>

class CBotWeapon
{
public:
	CBotWeapon(CBaseEntity* entity);
	~CBotWeapon();

	// True if the weapon entity is valid
	bool IsValid() const;

	/**
	 * @brief Gets the Weapon info.
	 * @return Weapon Info pointer or NULL if no weapon info was obtained from the weapon info manager.
	 */
	inline const std::shared_ptr<WeaponInfo>& GetWeaponInfo() const { return m_info; }
	inline const entities::HBaseCombatWeapon& GetBaseCombatWeapon() const { return m_bcw; }
	inline int GetWeaponEconIndex() const { return m_econindex; }
	inline int GetWeaponID() const { return m_weaponID; }
	template <typename T>
	inline T GetModWeaponID() const { return static_cast<T>(m_weaponID); }
	edict_t* GetEdict() const;
	CBaseEntity* GetEntity() const;
	int GetIndex() const;
private:
	CHandle<CBaseEntity> m_handle;
	std::shared_ptr<WeaponInfo> m_info;
	entities::HBaseCombatWeapon m_bcw;
	int m_econindex;
	int m_weaponID; // weapon ID, mod implemented
};

#endif // !NAVBOT_WEAPON_STORAGE_INTERFACE_H_
