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
}


#endif // !__NAVBOT_ZPS_LIB_H_
