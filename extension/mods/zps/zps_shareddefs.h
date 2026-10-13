#ifndef __NAVBOT_ZPS_SHAREDDEFS_H_
#define __NAVBOT_ZPS_SHAREDDEFS_H_

namespace zps
{
	enum ZPSTeam
	{
		ZPS_TEAM_UNASSIGNED = 0,
		ZPS_TEAM_SPECTATOR,
		ZPS_TEAM_SURVIVORS,
		ZPS_TEAM_ZOMBIES,

		MAX_ZPS_TEAMS
	};

	enum ZPSGamemodes
	{
		GAMEMODE_SURVIVAL = 0,
		GAMEMODE_OBJECTIVE,
		GAMEMODE_HARDCORE,

		MAX_ZPS_GAMEMODE_TYPES
	};

	/**
	 * @brief Values for item_deliver's m_iItemState member
	 */
	enum ItemDeliverStates
	{
		DELIVER_ITEM = 0,
		USEABLE_ITEM,
		FOOD_ITEM,

		MAX_ITEM_DELIVER_STATES
	};
}

// NavBot specific stuff
namespace zps::bots
{
	// Objective IDs for IsCompletingObjective queries
	// These can be any number , just make sure to use unique numbers.

	// Using a button (ZPO)
	constexpr int OBJID_USE_ENTITY = 100;
}


#endif // !__NAVBOT_ZPS_SHAREDDEFS_H_
