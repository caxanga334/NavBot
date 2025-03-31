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
}


#endif // !__NAVBOT_DODS_LIB_H_
