#ifndef __NAVBOT_DODS_LIB_H_
#define __NAVBOT_DODS_LIB_H_

#include "dods_shareddefs.h"

namespace dodslib
{
	dayofdefeatsource::DoDTeam GetDoDTeam(CBaseEntity* entity);
	dayofdefeatsource::DoDClassType GetPlayerClassType(CBaseEntity* player);
	/**
	 * @brief Given a class type and team, return the client command to select it.
	 * @param classtype Class to select.
	 * @param team From which team.
	 * @return Client command name to join the given class.
	 */
	const char* GetJoinClassCommand(dayofdefeatsource::DoDClassType classtype, dayofdefeatsource::DoDTeam team);
	/**
	 * @brief Reads the control point index from the entity's memory.
	 * @param entity Entity to read. Must be a dod_control_point.
	 * @return Control point index. Returns -1 on failure.
	 */
	int GetControlPointIndex(CBaseEntity* entity);
	dayofdefeatsource::DoDRoundState GetRoundState();

	inline const char* GetDoDTeamName(dayofdefeatsource::DoDTeam team)
	{
		switch (team)
		{
		case dayofdefeatsource::DODTEAM_SPECTATOR:
			return "SPECTATOR";
		case dayofdefeatsource::DODTEAM_ALLIES:
			return "ALLIES";
		case dayofdefeatsource::DODTEAM_AXIS:
			return "AXIS";
		default:
			return "UNASSIGNED";
		}
	}

	bool CanPlantBombAtTarget(CBaseEntity* bombTarget);
	bool CanTeamPlantBombAtTarget(CBaseEntity* bombTarget, int TeamNum);

	using DoDClassCountArray = std::array<int, dayofdefeatsource::DoDClassType::NUM_DOD_CLASSES>;

	/**
	 * @brief Fills an array with the number of players on each DoD class.
	 * @param team Team to get the class count of.
	 * @param classes Array to store the number of players as each class.
	 */
	void GetTeamClassCount(dayofdefeatsource::DoDTeam team, DoDClassCountArray& classes);
}


#endif // !__NAVBOT_DODS_LIB_H_
