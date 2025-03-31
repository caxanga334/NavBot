#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include "dodslib.h"

dayofdefeatsource::DoDTeam dodslib::GetDoDTeam(CBaseEntity* entity)
{
	return static_cast<dayofdefeatsource::DoDTeam>(entityprops::GetEntityTeamNum(entity));
}

dayofdefeatsource::DoDClassType dodslib::GetPlayerClassType(CBaseEntity* player)
{
	return  static_cast<dayofdefeatsource::DoDClassType>(entprops->GetCachedData<int>(player, CEntPropUtils::CDODPLAYER_PLAYERCLASS));
}

const char* dodslib::GetJoinClassCommand(dayofdefeatsource::DoDClassType classtype, dayofdefeatsource::DoDTeam team)
{
	if (team == dayofdefeatsource::DoDTeam::DODTEAM_ALLIES)
	{
		switch (classtype)
		{
		case dayofdefeatsource::DODCLASS_RIFLEMAN:
			return "cls_garand";
		case dayofdefeatsource::DODCLASS_ASSAULT:
			return "cls_tommy";
		case dayofdefeatsource::DODCLASS_SUPPORT:
			return "cls_bar";
		case dayofdefeatsource::DODCLASS_SNIPER:
			return "cls_spring";
		case dayofdefeatsource::DODCLASS_MACHINEGUNNER:
			return "cls_30cal";
		case dayofdefeatsource::DODCLASS_ROCKET:
			return "cls_bazooka";
		default:
			return "cls_random";
		}
	}
	else
	{
		switch (classtype)
		{
		case dayofdefeatsource::DODCLASS_RIFLEMAN:
			return "cls_k98";
		case dayofdefeatsource::DODCLASS_ASSAULT:
			return "cls_mp40";
		case dayofdefeatsource::DODCLASS_SUPPORT:
			return "cls_mp44";
		case dayofdefeatsource::DODCLASS_SNIPER:
			return "cls_k98s";
		case dayofdefeatsource::DODCLASS_MACHINEGUNNER:
			return "cls_mg42";
		case dayofdefeatsource::DODCLASS_ROCKET:
			return "cls_pschreck";
		default:
			return "cls_random";
		}
	}
}
