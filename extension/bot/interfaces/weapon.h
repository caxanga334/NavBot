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
	inline const WeaponInfo* GetWeaponInfo() const { return m_info; }
	inline const entities::HBaseCombatWeapon& GetBaseCombatWeapon() const { return m_bcw; }
	inline int GetWeaponEconIndex() const { return m_econindex; }
	edict_t* GetEdict() const;
	CBaseEntity* GetEntity() const;
	int GetIndex() const;
	/**
	 * @brief Checks if this weapon is running low on ammo.
	 * @param owner Bot that owns this weapon.
	 * @return True if this weapon is running low on ammo, false otherwise.
	 */
	bool IsAmmoLow(const CBaseBot* owner) const;
	/**
	 * @brief Checks if this weapon is out of ammo.
	 * @param owner Bot that owns this weapon.
	 * @return True if out of ammo, false otherwise.
	 */
	bool IsOutOfAmmo(const CBaseBot* owner) const;
	// Amount of primary ammo left.
	int GetPrimaryAmmoLeft(const CBaseBot* owner) const;
	// Amount of secondary ammo left.
	int GetSecondaryAmmoLeft(const CBaseBot* owner) const;
	// Total amount of ammo left in the weapon (primary + secondary ammo)
	int GetAmmoLeft(const CBaseBot* owner) const { return GetPrimaryAmmoLeft(owner) + GetSecondaryAmmoLeft(owner); }

	bool HasPrimaryAmmo(const CBaseBot* owner) const { return GetPrimaryAmmoLeft(owner) > 0; }
	bool hasSecondaryAmmo(const CBaseBot* owner) const { return GetSecondaryAmmoLeft(owner) > 0; }
	bool HasAmmo(const CBaseBot* owner) const { return !IsOutOfAmmo(owner); }
	bool IsInAttackRange(const float range, const WeaponInfo::AttackFunctionType attackType) const;
	// Returns true if it's possible to use the weapon's primary attack
	bool CanUsePrimaryAttack(const CBaseBot* owner) const;
	// Returns true if it's possible to use the weapon's secondary attack
	bool CanUseSecondaryAttack(const CBaseBot* owner) const;

	// True if this weapon's clip is full. Only checks primary clip ammo.
	bool IsClipFull() const;

	// Gets the minimum attack range based on the attack type used by the bot
	virtual float GetCurrentMinimumAttackRange(CBaseBot* owner) const;
	// Gets the maximum attack range based on the attack type used by the bot
	virtual float GetCurrentMaximumAttackRange(CBaseBot* owner) const;

	const std::string& GetClassname() const { return m_classname; }

protected:
	const WeaponInfo* m_info;

private:
	CHandle<CBaseEntity> m_handle;
	entities::HBaseCombatWeapon m_bcw;
	int m_econindex;
	std::string m_classname;
};

#endif // !NAVBOT_WEAPON_STORAGE_INTERFACE_H_
