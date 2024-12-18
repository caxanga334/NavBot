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
	inline bool HasAnyWeapons() { return !m_weapons.empty(); }

	/**
	 * @brief Runs a function on every valid bot weapon
	 * @tparam T a class with operator() overload with 1 parameter: (const CBotWeapon* weapon)
	 * @param functor function to run on every valid weapon
	 */
	template <typename T>
	inline void ForEveryWeapon(T functor) const
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
	// Gets the CBotWeapon pointer for the bot current active weapon. Can return NULL if the bot doesn't have an active weapon or the current weapon lacks a CBotWeapon.
	virtual std::shared_ptr<CBotWeapon> GetActiveBotWeapon();
	// Requests the bot inventory to be refreshed
	virtual void RequestRefresh() { m_updateWeaponsTimer.Invalidate(); }

	void OnWeaponEquip(CBaseEntity* weapon) override;

protected:
	std::vector<std::shared_ptr<CBotWeapon>> m_weapons;
	std::shared_ptr<CBotWeapon> m_cachedActiveWeapon;
	CountdownTimer m_updateWeaponsTimer;
	CountdownTimer m_weaponSwitchCooldown; // cooldown between weapon switches
};

#endif // !NAVBOT_INVENTORY_INTERFACE_H_
