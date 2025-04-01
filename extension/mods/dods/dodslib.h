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
}


#endif // !__NAVBOT_DODS_LIB_H_
