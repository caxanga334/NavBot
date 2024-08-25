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

	inline bool HasAnyWeapons() { return !m_weapons.empty(); }

	/**
	 * @brief Runs a function on every valid bot weapon
	 * @tparam T a class with operator() overload with 1 parameter: (const CBotWeapon& weapon)
	 * @param functor function to run on every valid weapon
	 */
	template <typename T>
	inline void ForEveryWeapon(T functor) const
	{
		for (const CBotWeapon& weapon : m_weapons)
		{
			if (!weapon.IsValid())
			{
				continue;
			}

			functor(weapon);
		}
	}

	/**
	 * @brief Checks if the bot has a weapon of the given classname.
	 * @param classname Weapon classname.
	 * @return true if the bot has the weapon.
	 */
	bool HasWeapon(std::string classname);

	/**
	 * @brief Builds the bot inventory of weapons and items.
	 */
	virtual void BuildInventory();

	virtual void SelectBestWeaponForThreat(const CKnownEntity* threat);

	virtual const CBotWeapon* GetActiveBotWeapon();

private:
	std::vector<CBotWeapon> m_weapons;

protected:
	CountdownTimer m_updateWeaponsTimer;
	CountdownTimer m_weaponSwitchCooldown; // cooldown between weapon switches
};

#endif // !NAVBOT_INVENTORY_INTERFACE_H_
