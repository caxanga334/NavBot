#ifndef __NAVBOT_CSS_LIB_H_
#define __NAVBOT_CSS_LIB_H_

#include "css_shareddefs.h"

namespace csslib
{
	/**
	 * @brief Gets an entity's CSS team.
	 * @param entity Entity to read the team from.
	 * @return Entity's CSS team.
	 */
	counterstrikesource::CSSTeam GetEntityCSSTeam(CBaseEntity* entity);
	/**
	 * @brief Gets the amount of money the given player has.
	 * @param player Player to read.
	 * @return Amount of money the given player has.
	 */
	int GetPlayerMoney(CBaseEntity* player);
	/**
	 * @brief Gets the player's "class" (skin).
	 * @param player Player to read.
	 * @return Player's class.
	 */
	int GetPlayerClass(CBaseEntity* player);
	/**
	 * @brief Checks if the given player is in a buy zone.
	 * @param player Player to read.
	 * @return True if the player is in a buy zone, false otherwise.
	 */
	bool IsInBuyZone(CBaseEntity* player);
	/**
	 * @brief Checks if the given player is in a bomb plant zone.
	 * @param player Player to read.
	 * @return True if the player is in a bomb plant zone, false otherwise.
	 */
	bool IsInBombPlantZone(CBaseEntity* player);
	/**
	 * @brief Checks if the given player is in a hostage rescue zone.
	 * @param player Player to read.
	 * @return True if the player is in a hostage rescue zone, false otherwise.
	 */
	bool IsInHostageRescueZone(CBaseEntity* player);
	/**
	 * @brief Checks if the given player has night vision goggles.
	 * @param player Player to read.
	 * @return True if the player has night vision goggles, false otherwise.
	 */
	bool HasNightVisionGoggles(CBaseEntity* player);
	/**
	 * @brief Checks if the given player has helmet equipped.
	 * @param player Player to read.
	 * @return True if the player has helmet equipped, false otherwise.
	 */
	bool HasHelmetEquipped(CBaseEntity* player);
	/**
	 * @brief Checks if the given player has a defuse kit.
	 * @param player Player to read.
	 * @return True if the player has a defuse kit, false otherwise.
	 */
	bool HasDefuser(CBaseEntity* player);
	/**
	 * @brief Checks if the given player is defusing the bomb.
	 * @param player Player to read.
	 * @return True if the player is defusing, false otherwise.
	 */
	bool IsDefusing(CBaseEntity* player);
	/**
	 * @brief Gets the amount of time remaining until the c4 explodes.
	 * @param c4 C4 entity (planted_c4).
	 * @return Amount of time left until the c4 explodes in seconds.
	 */
	float GetC4TimeRemaining(CBaseEntity* c4);
	/**
	 * @brief Checks if the given hostage is rescued.
	 * @param hostage Hostage to check. (hostage_entity)
	 * @return True if the given hostage was rescued, false otherwise.
	 */
	bool IsHostageRescued(CBaseEntity* hostage);
	/**
	 * @brief Gets the player leading the given hostage.
	 * @param hostage Hostage to read the leader of.
	 * @return Player leading the hostage, NULL if none.
	 */
	CBaseEntity* GetHostageLeader(CBaseEntity* hostage);
	// Returns true if freeze time is active.
	bool IsInFreezeTime();
	// Returns true if the current map has bomb targets.
	bool MapHasBombTargets();
	// Returns true if the current map has hostage rescue zones.
	bool MapHasHostageRescueZones();
	// Returns the number of hostages remaining in the map.
	int GetNumberOfRemainingHostages();
	// Returns the timestamp of when the current round started. (Freeze time not counted)
	float GetRoundStartTime();
	// Returns the amount of time in seconds remaining for the current round.
	float GetRoundTimeRemaining();
}

#endif // !__NAVBOT_CSS_LIB_H_
