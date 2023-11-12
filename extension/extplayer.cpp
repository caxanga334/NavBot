#include "extension.h"
#include "navmesh/nav_mesh.h"
#include "extplayer.h"

extern CGlobalVars* gpGlobals;
extern IPlayerInfoManager* playerinfomanager;
extern IServerGameClients* gameclients;
extern CNavMesh* TheNavMesh;

constexpr auto PLAYER_NAV_UPDATE_TIME = 0.6f; // Interval of nav mesh data updates
constexpr auto NEAREST_AREA_MAX_DISTANCE = 128.0f;

void CBaseExtPlayer::PlayerThink()
{
	if (m_playerinfo == nullptr)
	{
		m_playerinfo = playerinfomanager->GetPlayerInfo(m_edict);
		return;
	}

	if (m_navupdatetimer > 0)
	{
		m_navupdatetimer--;
	}
	else
	{
		m_navupdatetimer = TIME_TO_TICKS(PLAYER_NAV_UPDATE_TIME);
		auto origin = GetAbsOrigin();
		auto newarea = TheNavMesh->GetNearestNavArea(origin, NEAREST_AREA_MAX_DISTANCE, false, true, m_playerinfo->GetTeamIndex());

		if (newarea != m_lastnavarea)
		{
			OnNavAreaChanged(m_lastnavarea, newarea);
			m_lastnavarea = newarea;
		}
	}
}

const Vector CBaseExtPlayer::GetAbsOrigin()
{
	return m_playerinfo->GetAbsOrigin();
}

const QAngle CBaseExtPlayer::GetAbsAngles()
{
	return m_playerinfo->GetAbsAngles();
}

const Vector CBaseExtPlayer::GetEyeOrigin()
{
	Vector ear;
	gameclients->ClientEarPosition(m_edict, &ear);
	return ear;
}

const QAngle CBaseExtPlayer::GetEyeAngles()
{
	// TO-DO: This is the 'RCBot' way of getting eye angles
	// Sourcemod VCalls EyeAngles.
	// Determine which is better

	auto cmd = m_playerinfo->GetLastUserCommand();
	return cmd.viewangles;
}

CON_COMMAND(smnav_debug_vectors, "[LISTEN SERVER] Debug player vectors")
{

}