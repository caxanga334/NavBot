#ifndef __NAVBOT_ZPS_LIB_H_
#define __NAVBOT_ZPS_LIB_H_

#include "zps_shareddefs.h"

namespace zpslib
{
	zps::ZPSTeam GetEntityZPSTeam(CBaseEntity* entity);
	bool IsPlayerInfected(CBaseEntity* player);
	bool IsPlayerWalking(CBaseEntity* player);
	bool IsPlayerCarrier(CBaseEntity* player);
	CBaseEntity* GetCarrierZombie();
	/**
	 * @brief Gets a random living survivor player.
	 * @param nobots If true, ignore bots.
	 * @return Random survivor. NULL if none matches.
	 */
	CBaseEntity* GetRandomLivingSurvivor(const bool nobots);
	// Checks if a trigger_useable is disabled
	bool IsUseableTriggerDisabled(CBaseEntity* entity);
	// Gets the item state of a item_deliver
	zps::ItemDeliverStates GetItemDeliverState(CBaseEntity* entity);
	std::string GetItemDeliverIDName(CBaseEntity* entity);
	float GetPlayerWeight(CBaseEntity* player);
	// Checks if the given player owns an item_deliver with the given ID name
	bool PlayerHasNamedItem(CBaseEntity* player, const char* id);
}


#endif // !__NAVBOT_ZPS_LIB_H_
