#ifndef NAVBOT_WEAPON_STORAGE_INTERFACE_H_
#define NAVBOT_WEAPON_STORAGE_INTERFACE_H_
#pragma once

#include <string>
#include <sdkports/sdk_ehandle.h>
#include "weaponinfo.h"
#include <entities/basecombatweapon.h>

class CBaseBot;
class CKnownEntity;

class CBotWeapon
{
public:
	CBotWeapon(CBaseEntity* entity);
	virtual ~CBotWeapon();

	// True if the weapon entity is valid
	bool IsValid() const;
	// true if the given bot is this weapon's owner.
	bool IsOwnedByBot(const CBaseBot* bot) const;
	/**
	 * @brief Gets the Weapon info.
	 * @return Weapon Info pointer or NULL if no weapon info was obtained from the weapon info manager.
	 */
	inline const WeaponInfo* GetWeaponInfo() const { return m_info; }
	inline const entities::HBaseCombatWeapon& GetBaseCombatWeapon() const { return m_bcw; }
	inline int GetWeaponEconIndex() const { return m_econindex; }
	edict_t* GetEdict() const;
	// Weapon's CBaseEntity pointer
	CBaseEntity* GetEntity() const { return m_handle.Get(); }
	// Weapon's entity index
	int GetIndex() const { return m_entindex; }
	// Returns true if this weapon matches the given classname
	bool IsWeapon(const char* classname) const;
	/**
	 * @brief Returns true if the weapon's entity classname matches the given pattern.
	 * 
	 * Example: ClassnameMatchesPattern("tf_weapon_sniper*")
	 * @param pattern Pattern to match.
	 * @return True if the given pattern matches. False otherwise.
	 */
	const bool ClassnameMatchesPattern(const char* pattern) const;
	// Returns true if this weapon matches the given econ index
	bool IsWeapon(const int econindex) const { return econindex == m_econindex; }
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
	// Returns true if the weapon has ammo loaded in the clip.
	// If the weapon doesn't uses clips, also returns true.
	bool IsLoaded() const;

	// Gets the minimum attack range based on the attack type used by the bot
	virtual float GetCurrentMinimumAttackRange(CBaseBot* owner) const;
	// Gets the maximum attack range based on the attack type used by the bot
	virtual float GetCurrentMaximumAttackRange(CBaseBot* owner) const;
	// Gets the weapon's entity classname (cached)
	const std::string& GetClassname() const { return m_classname; }
	/**
	 * @brief Gets the dynamic weapon selection priority for this weapon.
	 * @param owner Bot who owns this weapon.
	 * @param range Optional: Range to threat
	 * @param threat Optional: Current threat
	 * @return Weapon selection priority
	 */
	const int GetPriority(const CBaseBot* owner, const float* range = nullptr, const CKnownEntity* threat = nullptr) const;
	// returns true if the weapon is deployed/scoped.
	virtual bool IsDeployedOrScoped(const CBaseBot* owner) const;
	// returns true if the weapon's special function can be used.
	bool CanUseSpecialFunction(const CBaseBot* owner, const float range) const;
	// Weapon debug identifier string
	const char* GetDebugIdentifier() const;
	// Returns the weapon subtype. Used when selecting weapons via UserCmd
	virtual int GetSubType() const;
protected:
	const WeaponInfo* m_info;

	float GetCustomAmmo(const CBaseBot* owner) const;

private:
	CHandle<CBaseEntity> m_handle;
	entities::HBaseCombatWeapon m_bcw;
	int m_econindex;
	int m_entindex;
	std::string m_classname;
};

#endif // !NAVBOT_WEAPON_STORAGE_INTERFACE_H_
