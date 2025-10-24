#include NAVBOT_PCH_FILE
#include "zpsbot.h"
#include "zpsbot_movement.h"

CZPSBotMovement::CZPSBotMovement(CZPSBot* bot) :
	IMovement(bot)
{
}

bool CZPSBotMovement::IsUseableObstacle(CBaseEntity* entity)
{
	CZPSBot* bot = GetBot<CZPSBot>();

	// Zombies can't open doors
	if (bot->GetMyZPSTeam() == zps::ZPSTeam::ZPS_TEAM_ZOMBIES)
	{
		return false;
	}

	return IMovement::IsUseableObstacle(entity);
}
