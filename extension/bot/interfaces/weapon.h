#ifndef NAVBOT_WEAPON_STORAGE_INTERFACE_H_
#define NAVBOT_WEAPON_STORAGE_INTERFACE_H_
#pragma once

#include <string>
#include <sdkports/sdk_ehandle.h>
#include "weaponinfo.h"
#include <entities/basecombatweapon.h>

class CBaseBot;

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
	/**
	 * @brief Checks if this weapon is running low on ammo.
	 * @param owner Bot that owns this weapon.
	 * @param primaryOnly If true, ignores secondary ammo.
	 * @param lowThresholdOverride If non zero and positive, overrides the low ammo threshold from the weapon info config.
	 * @return True if this weapon is running low on ammo, false otherwise.
	 */
	bool IsAmmoLow(const CBaseBot* owner, const bool primaryOnly, const int lowThresholdOverride = 0) const;
	/**
	 * @brief Checks if this weapon is out of ammo.
	 * @param owner Bot that owns this weapon.
	 * @param inClipOnly If true, only checks the weapon clip and ignores reserve ammo.
	 * @param primaryOnly If true, only checks the primary ammo.
	 * @return True if out of ammo, false otherwise.
	 */
	bool IsOutOfAmmo(const CBaseBot* owner, const bool inClipOnly, const bool primaryOnly) const;
	// True if this weapon's clip is full. Only checks primary clip ammo.
	bool IsClipFull() const;

	// Gets the minimum attack range based on the attack type used by the bot
	virtual float GetCurrentMinimumAttackRange(CBaseBot* owner) const;
	// Gets the maximum attack range based on the attack type used by the bot
	virtual float GetCurrentMaximumAttackRange(CBaseBot* owner) const;

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
