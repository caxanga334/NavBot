#include NAVBOT_PCH_FILE
#include "zpsbot.h"
#include <mods/zps/nav/zps_nav_mesh.h>
#include <coordsize.h>
#include "zpsbot_movement.h"


CZPSBotMovement::CZPSBotMovement(CZPSBot* bot) :
	IMovement(bot)
{
}

bool CZPSBotMovement::IsUseableObstacle(CBaseEntity* entity, CBaseEntity** useTarget)
{
	CZPSBot* bot = GetBot<CZPSBot>();

	if (bot->GetMyZPSTeam() == zps::ZPSTeam::ZPS_TEAM_ZOMBIES)
	{
		// Zombies can't use open prop doors
		if (entprops->HasEntProp(entity, Prop_Data, "m_eDoorState"))
		{
			return false;
		}
	}

	return IMovement::IsUseableObstacle(entity, useTarget);
}

int CZPSBotMovement::GetMovementCollisionGroup() const
{
	return static_cast<int>(Collision_Group_t::COLLISION_GROUP_PLAYER);
}

float CZPSBotMovement::GetMaxDropHeight() const
{
	CZPSBot* bot = GetBot<CZPSBot>();

	switch (bot->GetMyZPSTeam())
	{
	case zps::ZPSTeam::ZPS_TEAM_ZOMBIES:
		// zombies have a max fall damage cap which allows them to survive great falls
		if (bot->GetHealth() > 50)
		{
			return MAX_COORD_FLOAT;
		}

		return IMovement::GetMaxDropHeight();
	default:
		return IMovement::GetMaxDropHeight();
	}
}

void CZPSBotMovement::UpdateMovementButtons()
{
	if (IsWalking())
	{
		GetBot<CZPSBot>()->GetControlInterface()->PressWalkButton();
	}
}
