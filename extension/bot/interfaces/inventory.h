#ifndef NAVBOT_INVENTORY_INTERFACE_H_
#define NAVBOT_INVENTORY_INTERFACE_H_

#include <unordered_map>
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
	inline bool HasAnyWeapons() const { return !m_weapons.empty(); }

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
			if (!weapon->IsValid())
			{
				continue;
			}

			functor(weapon.get());
		}
	}

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
	 */
	virtual void SelectBestWeaponForThreat(const CKnownEntity* threat);
	/**
	 * @brief Selects the best weapon from the bot inventory.
	 */
	virtual void SelectBestWeapon();
	/**
	 * @brief Selects the best weapon from the bot inventory to attack breakable entities (func_breakable, prop_physics, etc...)
	 */
	virtual void SelectBestWeaponForBreakables();
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

protected:
	std::vector<std::unique_ptr<CBotWeapon>> m_weapons;
	mutable CBotWeapon* m_cachedActiveWeapon;
	CountdownTimer m_updateWeaponsTimer;
	CountdownTimer m_purgeStaleWeaponsTimer;
	CountdownTimer m_weaponSwitchCooldown; // cooldown between weapon switches
};

#endif // !NAVBOT_INVENTORY_INTERFACE_H_
