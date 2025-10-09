#ifndef NAVBOT_INVENTORY_INTERFACE_H_
#define NAVBOT_INVENTORY_INTERFACE_H_

#include <unordered_map>
#include <cstring>
#include <sdkports/sdk_timers.h>
#include "base_interface.h"
#include "weapon.h"

class CKnownEntity;

/**
 * @brief Inventory interface. Responsible for managing and keeping track of the bot inventory of items and weapons.
 */
class IInventory : public IBotInterface
{
public:
	IInventory(CBaseBot* bot);
	~IInventory() override;

	// default 1 second cooldown between weapon switches
	static constexpr float base_weapon_switch_cooldown() { return 1.0f; }

	// Reset the interface to it's initial state
	void Reset() override;
	// Called at intervals
	void Update() override;
	// Called every server frame
	void Frame() override;
	// Called to notify that the weapon info configuration file was reloaded.
	virtual void OnWeaponInfoConfigReloaded();
protected:
	// Creates a new bot weapon object, override to use mod specific class
	virtual CBotWeapon* CreateBotWeapon(CBaseEntity* weapon) { return new CBotWeapon(weapon); }

public:
	// Returns true if the bot has at least one weapon in the inventory (doesn't validate the weapons)
	inline bool HasAnyWeapons() const { return !m_weapons.empty(); }
	/**
	 * @brief Checks if the bot contains at least one weapon of the given type in their inventory.
	 * @param type Weapon type to search for.
	 * @param validate If true, check that the weapon entity is valid and owned by the bot.
	 * @return True if at least a single weapon is found, false otherwise.
	 */
	inline bool HasAnyWeaponOfType(WeaponInfo::WeaponType type, const bool validate = true) const
	{
		CBaseBot* bot = GetBot<CBaseBot>();

		for (auto& weapon : m_weapons)
		{
			if (validate)
			{
				if (!weapon->IsValid() || !weapon->IsOwnedByBot(bot)) { continue; }
			}

			if (weapon->GetWeaponInfo()->GetWeaponType() == type)
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Runs a function on every valid bot weapon
	 * @tparam T a class with operator() overload with 1 parameter: (const CBotWeapon* weapon)
	 * @param functor function to run on every valid weapon
	 */
	template <typename T>
	inline void ForEveryWeapon(T& functor) const
	{
		for (auto& weapon : m_weapons)
		{
			if (!weapon->IsValid() || !weapon->IsOwnedByBot(GetBot<CBaseBot>()))
			{
				continue;
			}

			functor(weapon.get());
		}
	}

	/**
	 * @brief Finds a weapon by their entity classname.
	 * @param classname Weapon's entity classname to search for;
	 * @param validateOwnership If true, check that the bot still owns the weapon.
	 * @return Bot Weapon interface pointer or NULL if not found.
	 */
	const CBotWeapon* FindWeaponByClassname(const char* classname, const bool validateOwnership = true) const
	{
		for (auto& weaponptr : m_weapons)
		{
			if (weaponptr->IsValid() && std::strcmp(weaponptr->GetClassname().c_str(), classname) == 0)
			{
				if (validateOwnership && !weaponptr->IsOwnedByBot(GetBot<CBaseBot>()))
				{
					continue;
				}

				return weaponptr.get();
			}
		}

		return nullptr;
	}

	/**
	 * @brief Finds a weapon by their entity classname. Supports patterns with *
	 * 
	 * Example: FindWeaponByClassnamePattern("tf_weapon_sniper*")
	 * @param pattern Pattern to search for.
	 * @param validateOwnership If true, check if the weapon is still owned by the bot.
	 * @return Bot Weapon interface pointer or NULL if not found.
	 */
	const CBotWeapon* FindWeaponByClassnamePattern(const char* pattern, const bool validateOwnership = true) const;

	/**
	 * @brief Finds a weapon by their economy index.
	 * @param index Weapon's economy item index.
	 * @param validateOwnership If true, check if the weapon is still owned by the bot.
	 * @return Bot Weapon interface pointer or NULL if not found.
	 */
	const CBotWeapon* FindWeaponByEconIndex(const int index, const bool validateOwnership = true) const
	{
		for (auto& weaponptr : m_weapons)
		{
			if (weaponptr->IsValid() && weaponptr->GetWeaponEconIndex() == index)
			{
				if (validateOwnership && !weaponptr->IsOwnedByBot(GetBot<CBaseBot>()))
				{
					continue;
				}

				return weaponptr.get();
			}
		}

		return nullptr;
	}

	/**
	 * @brief Finds the first weapon that contains the given tag.
	 * @param tag Tag to search.
	 * @param validateOwnership If true, check if the weapon is still owned by the bot.
	 * @return Bot Weapon interface pointer or NULL if not found.
	 */
	const CBotWeapon* FindWeaponByTag(const std::string& tag, const bool validateOwnership = true) const
	{
		for (auto& weaponptr : m_weapons)
		{
			if (weaponptr->IsValid() && weaponptr->GetWeaponInfo()->HasTag(tag))
			{
				if (validateOwnership && !weaponptr->IsOwnedByBot(GetBot<CBaseBot>()))
				{
					continue;
				}

				return weaponptr.get();
			}
		}

		return nullptr;
	}

	/**
	 * @brief Finds the first weapon of the given weapon type.
	 * @param type Weapon type to search for.
	 * @param validateOwnership If true, check if the weapon is still owned by the bot.
	 * @return Bot Weapon interface pointer or NULL if not found.
	 */
	const CBotWeapon* FindWeaponByType(WeaponInfo::WeaponType type, const bool validateOwnership = true) const
	{
		for (auto& weaponptr : m_weapons)
		{
			if (weaponptr->IsValid() && weaponptr->GetWeaponInfo()->GetWeaponType() == type)
			{
				if (validateOwnership && !weaponptr->IsOwnedByBot(GetBot<CBaseBot>()))
				{
					continue;
				}

				return weaponptr.get();
			}
		}

		return nullptr;
	}

	/**
	 * @brief Checks if the current active weapon is the given weapon, if it's not, equip the given weapon.
	 * @param weapon Weapon to equip.
	 * @return Returns true if the weapon was equipped.
	 */
	bool EquipWeapon(const CBotWeapon* weapon) const;
	/**
	 * @brief Checks if the bot has a weapon of the given classname.
	 * @param classname Weapon classname.
	 * @return true if the bot has the weapon.
	 */
	bool HasWeapon(const char* classname);
	/**
	 * @brief Checks if the bot owns this weapon.
	 * @param weapon Weapon entity;
	 * @return true if the bot owns this weapon.
	 */
	bool HasWeapon(CBaseEntity* weapon);

	/**
	 * @brief Gets a weapon CBaseEntity pointer (if owned by the bot)
	 * @param classname Weapon classname to search.
	 * @return Weapon entity pointer or NULL if the bot doesn't own a weapon of the given classname.
	 */
	CBaseEntity* GetWeaponEntity(const char* classname);

	/**
	 * @brief Given a weapon entity, returns a bot weapon interface pointer.
	 * @param weapon Weapon entity.
	 * @return Bot weapon interface pointer.
	 */
	const CBotWeapon* GetWeaponOfEntity(CBaseEntity* weapon);

	/**
	 * @brief Registers the given weapon to the bot inventory.
	 * @param weapon Weapon entity.
	 */
	virtual void AddWeaponToInventory(CBaseEntity* weapon);

	/**
	 * @brief Builds the bot inventory of weapons and items.
	 */
	virtual void BuildInventory();
	/**
	 * @brief Given a known threat, selects the best weapon from the current inventory.
	 * @param threat Threat, NULL is NOT accepted.
	 * @param typeOnly Optional weapon type filter. Leave at default value for any weapon useable in combat.
	 * @return True if a weapon the weapon was selected, false otherwise.
	 */
	virtual bool SelectBestWeaponForThreat(const CKnownEntity* threat, WeaponInfo::WeaponType typeOnly = WeaponInfo::WeaponType::MAX_WEAPON_TYPES);
	/**
	 * @brief Selects the best weapon from the bot inventory.
	 */
	virtual void SelectBestWeapon();
	/**
	 * @brief Selects the best weapon from the bot inventory to attack breakable entities (func_breakable, prop_physics, etc...)
	 */
	virtual void SelectBestWeaponForBreakables();
	/**
	 * @brief Selects the weapon with the highest priority that matches the given type.
	 * @param type Weapon Type to filter.
	 * @param range Optional, if positive, also filter weapons by range.
	 */
	virtual void SelectBestWeaponOfType(WeaponInfo::WeaponType type, const float range = -1.0f);
	/**
	 * @brief Selects the best hitscan weapon from the bot inventory.
	 * @param meleeIsHitscan If true, consider melee weapon as hitscan.
	 */
	virtual void SelectBestHitscanWeapon(const bool meleeIsHitscan = true);
	// Gets the CBotWeapon pointer for the bot current active weapon. Can return NULL if the bot doesn't have an active weapon or the current weapon lacks a CBotWeapon.
	virtual const CBotWeapon* GetActiveBotWeapon() const;
	// Requests the bot inventory to be refreshed
	virtual void RequestRefresh() { m_updateWeaponsTimer.Invalidate(); }
	/**
	 * @brief @brief Checks if any weapon has low ammo.
	 * @param heldOnly If true, only checks the currently held weapon.
	 * @return True if the bot contains any weapons with low ammo. False otherwise.
	 */
	virtual bool IsAmmoLow(const bool heldOnly = true);

	void OnWeaponEquip(CBaseEntity* weapon) override;
	
	// Number of valid weapons this bot owns
	int GetOwnedWeaponCount() const;
	// Forces the stale weapons removal to run the next Update
	void ForceStaleWeaponsCheck() { m_purgeStaleWeaponsTimer.Invalidate(); }
	// returns the vector that stores the weapons
	const std::vector<std::unique_ptr<CBotWeapon>>& GetAllWeapons() const { return m_weapons; }
	const bool IsWeaponSwitchInCooldown() const { return m_weaponSwitchCooldown.HasStarted() && !m_weaponSwitchCooldown.IsElapsed(); }
	void StartWeaponSwitchCooldown(const float time = 0.5f) { m_weaponSwitchCooldown.Start(time); }

protected:
	// deletes invalid weapons from the weapon storage vector
	void RemoveInvalidWeapons();

	void SetActiveWeaponCache(CBotWeapon* weapon) const { m_cachedActiveWeapon = weapon; }

	/**
	 * @brief Invoked to filter the best weapon to be used against the given threat.
	 * @param me The bot itself.
	 * @param threat The threat.
	 * @param rangeToThreat Distance between the bot and the threat.
	 * @param first The first weapon.
	 * @param second The second weapon.
	 * @return The weapon the bot should use. NULL for none and end the loop.
	 */
	virtual const CBotWeapon* FilterBestWeaponForThreat(CBaseBot* me, const CKnownEntity* threat, const float rangeToThreat, const CBotWeapon* first, const CBotWeapon* second) const;
	// Returns true if the weapon can be used agains the given threat
	bool IsWeaponUseableForThreat(CBaseBot* me, const CKnownEntity* threat, const float rangeToThreat, const CBotWeapon* weapon, const WeaponInfo* info) const;
	// Selects the weapon with the highest static priority (no dynamic priority support)
	const CBotWeapon* FilterSelectWeaponWithHighestStaticPriority(const CBotWeapon* first, const CBotWeapon* second) const;
	/**
	 * @brief Invoked when a new weapon is selected by the bot.
	 * @param weapon Weapon the bot just selected.
	 */
	virtual void OnBotWeaponEquipped(const CBotWeapon* weapon) const {}
private:
	std::vector<std::unique_ptr<CBotWeapon>> m_weapons;
	mutable CBotWeapon* m_cachedActiveWeapon;
	CountdownTimer m_updateWeaponsTimer;
	CountdownTimer m_purgeStaleWeaponsTimer;
	CountdownTimer m_weaponSwitchCooldown; // cooldown between weapon switches
};

#endif // !NAVBOT_INVENTORY_INTERFACE_H_
