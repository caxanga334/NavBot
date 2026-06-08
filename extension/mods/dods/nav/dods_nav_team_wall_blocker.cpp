#include NAVBOT_PCH_FILE
#include "../dodslib.h"
#include "dods_nav_mesh.h"
#include "dods_nav_area.h"
#include "dods_nav_team_wall_blocker.h"

CDoDSNavTeamWallBlocker::CDoDSNavTeamWallBlocker(CBaseEntity* entity) :
	m_entity(entity)
{
	m_isNewBlocker = UtilHelpers::FClassnameIs(entity, "func_teamblocker");
	m_iTeamNum = NAV_TEAM_ANY;
	UpdateTeam();
	FindAreas();
}

void CDoDSNavTeamWallBlocker::Update()
{
	UpdateTeam();

	if (m_iTeamNum <= static_cast<int>(dayofdefeatsource::DoDTeam::DODTEAM_SPECTATOR))
	{
		Invalidate();
	}
}

void CDoDSNavTeamWallBlocker::PrintDebugInfo() const
{
	META_CONPRINTF("Entity: %s \n", UtilHelpers::textformat::FormatEntity(m_entity.Get()));
}

void CDoDSNavTeamWallBlocker::UpdateTeam()
{
	if (m_isNewBlocker)
	{
		entprops->GetEntProp(m_entity.Get(), Prop_Send, "m_iTeamNum", m_iTeamNum);
	}
	else
	{
		entprops->GetEntProp(m_entity.Get(), Prop_Data, "m_iBlockTeam", m_iTeamNum);
	}
}

void CDoDSNavTeamWallBlocker::FindAreas()
{
	AddAreasTouchingEntity(m_entity.Get(), navgenparams->step_height, navgenparams->half_human_width);

	if (IsAreaVectorEmpty())
	{
		Invalidate();
	}
}
