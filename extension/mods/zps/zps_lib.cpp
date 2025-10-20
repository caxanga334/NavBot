#include NAVBOT_PCH_FILE
#include "zps_lib.h"

zps::ZPSTeam zpslib::GetEntityZPSTeam(CBaseEntity* entity)
{
	int team = 0;
	entprops->GetEntProp(entity, Prop_Data, "m_iTeamNum", team);
	return static_cast<zps::ZPSTeam>(team);
}
