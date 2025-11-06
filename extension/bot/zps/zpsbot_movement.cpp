#include NAVBOT_PCH_FILE
#include "zpsbot.h"
#include <mods/zps/nav/zps_nav_mesh.h>
#include "zpsbot_movement.h"


CZPSBotMovement::CZPSBotMovement(CZPSBot* bot) :
	IMovement(bot)
{
}

bool CZPSBotMovement::IsUseableObstacle(CBaseEntity* entity)
{
	CZPSBot* bot = GetBot<CZPSBot>();

	// Zombies can't use open doors
	if (bot->GetMyZPSTeam() == zps::ZPSTeam::ZPS_TEAM_ZOMBIES)
	{
		return false;
	}

	return IMovement::IsUseableObstacle(entity);
}

int CZPSBotMovement::GetMovementCollisionGroup() const
{
	return static_cast<int>(Collision_Group_t::COLLISION_GROUP_PLAYER);
}

bool CZPSBotMovement::IsAreaTraversable(const CNavArea* area) const
{
	bool base = IMovement::IsAreaTraversable(area);

	if (base)
	{
		const CZPSNavArea* zparea = static_cast<const CZPSNavArea*>(area);
		zps::ZPSTeam myteam = GetBot<CZPSBot>()->GetMyZPSTeam();

		if (myteam == zps::ZPSTeam::ZPS_TEAM_ZOMBIES && zparea->HasZPSAttributes(CZPSNavArea::ZPS_ATTRIBUTE_NOZOMBIES))
		{
			return false;
		}

		if (myteam == zps::ZPSTeam::ZPS_TEAM_SURVIVORS && zparea->HasZPSAttributes(CZPSNavArea::ZPS_ATTRIBUTE_NOHUMANS))
		{
			return false;
		}
	}

	return base;
}

void CZPSBotMovement::UpdateMovementButtons()
{
	if (IsWalking())
	{
		GetBot<CZPSBot>()->GetControlInterface()->PressWalkButton();
	}
}
