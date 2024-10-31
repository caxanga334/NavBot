#include <vector>
#include <extension.h>
#include <mods/tf2/teamfortress2mod.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/debugoverlay_shared.h>
#include "tfnavmesh.h"
#include "tfnav_waypoint.h"

CTFWaypoint::CTFWaypoint()
{
	m_cpindex = CTFWaypoint::NO_CONTROL_POINT;
}

CTFWaypoint::~CTFWaypoint()
{
}

void CTFWaypoint::Save(std::fstream& filestream, uint32_t version)
{
	CWaypoint::Save(filestream, version);

	filestream.write(reinterpret_cast<char*>(&m_cpindex), sizeof(int));
}

NavErrorType CTFWaypoint::Load(std::fstream& filestream, uint32_t version, uint32_t subVersion)
{
	NavErrorType base = CWaypoint::Load(filestream, version, subVersion);

	if (base != NAV_OK)
	{
		return base;
	}

	if (!filestream.good())
	{
		return NAV_CORRUPT_DATA;
	}

	filestream.read(reinterpret_cast<char*>(&m_cpindex), sizeof(int));

	return filestream.good() ? NAV_OK : NAV_CORRUPT_DATA;
}

bool CTFWaypoint::IsAvailableToTeam(const int teamNum)
{
	using namespace tfentities;

	// If a control point is assigned to this waypoint
	if (m_cpindex != CTFWaypoint::NO_CONTROL_POINT && (GetTeam() == NAV_TEAM_ANY || GetTeam() == teamNum))
	{
		std::vector<CBaseEntity*> controlpoints;
		controlpoints.reserve(8);
		TeamFortress2::TFTeam team = static_cast<TeamFortress2::TFTeam>(teamNum);

		CTeamFortress2Mod::GetTF2Mod()->CollectControlPointsToAttack(team, controlpoints);
		CTeamFortress2Mod::GetTF2Mod()->CollectControlPointsToDefend(team, controlpoints);

		// A waypoit will be available if it can be attacked or defended by the given team

		for (CBaseEntity* pEntity : controlpoints)
		{
			HTeamControlPoint cp(pEntity);

			if (cp.GetPointIndex() == m_cpindex)
			{
				return true;
			}
		}

		return false;
	}

	// Else just use base
	return CWaypoint::IsAvailableToTeam(teamNum);
}

void CTFWaypoint::DrawModText() const
{
	NDebugOverlay::Text(GetOrigin() + Vector(0.0f, 0.0f, CWaypoint::WAYPOINT_MOD_TEXT_HEIGHT), false, NDEBUG_PERSIST_FOR_ONE_TICK, "CP %i", m_cpindex);
}
