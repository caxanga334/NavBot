#include NAVBOT_PCH_FILE
#include <mods/dods/dods_shareddefs.h>
#include <mods/dods/dayofdefeatsourcemod.h>
#include <mods/dods/dodslib.h>
#include "dods_nav_mesh.h"
#include "dods_nav_area.h"
#include "dods_nav_waypoint.h"

CDoDSWaypoint::CDoDSWaypoint() :
	CWaypoint(), m_dodattributes(0), m_CPIndex(dayofdefeatsource::INVALID_CONTROL_POINT)
{
}

void CDoDSWaypoint::Save(std::fstream& filestream, uint32_t version)
{
	CWaypoint::Save(filestream, version);

	filestream.write(reinterpret_cast<char*>(&m_dodattributes), sizeof(int));
	filestream.write(reinterpret_cast<char*>(&m_CPIndex), sizeof(int));
}

NavErrorType CDoDSWaypoint::Load(std::fstream& filestream, uint32_t version, uint32_t subVersion)
{
	NavErrorType baseCode = CWaypoint::Load(filestream, version, subVersion);

	if (baseCode != NavErrorType::NAV_OK)
	{
		return baseCode;
	}

	/* version 3: Added dod specific waypoint data */
	if (subVersion >= 3)
	{
		filestream.read(reinterpret_cast<char*>(&m_dodattributes), sizeof(int));
		filestream.read(reinterpret_cast<char*>(&m_CPIndex), sizeof(int));
	}

	if (filestream.bad())
	{
		return NAV_CORRUPT_DATA;
	}

	return NavErrorType::NAV_OK;
}

bool CDoDSWaypoint::IsAssignedToaControlPoint() const
{
	return m_CPIndex != dayofdefeatsource::INVALID_CONTROL_POINT;
}

bool CDoDSWaypoint::IsAvailableToTeam(const int teamNum) const
{
	// first check base class
	if (CWaypoint::IsAvailableToTeam(teamNum))
	{
		if (IsAssignedToaControlPoint())
		{
			CDayOfDefeatSourceMod* mod = CDayOfDefeatSourceMod::GetDODMod();
			const CDODObjectiveResource* objres = mod->GetDODObjectiveResource();

			if (objres)
			{
				/* Owners only: the waypoint is only available if the given team number matches the owner's team number. */
				if (HasDoDWaypointAttributes(DOD_WP_ATTRIBS_POINT_OWNERS_ONLY))
				{
					return objres->GetOwnerTeamIndex(m_CPIndex) == teamNum;
				}

				/* The inverse of owners only */
				if (HasDoDWaypointAttributes(DOD_WP_ATTRIBS_POINT_ATTACKERS_ONLY))
				{
					return objres->GetOwnerTeamIndex(m_CPIndex) != teamNum;
				}
			}
			
			std::vector<const CDayOfDefeatSourceMod::DoDControlPoint*> points;

			mod->CollectControlPointsToAttack(static_cast<dayofdefeatsource::DoDTeam>(teamNum), points);
			mod->CollectControlPointsToDefend(static_cast<dayofdefeatsource::DoDTeam>(teamNum), points);

			for (const CDayOfDefeatSourceMod::DoDControlPoint* point : points)
			{
				if (m_CPIndex == point->index)
				{
					return true;
				}
			}
			
			return false;
		}

		return true;
	}

	return false;
}

CDoDSWaypoint::DoDWPAttributes CDoDSWaypoint::StringToDoDWPAttribute(const char* str)
{
	for (auto& pair : CDoDSWaypoint::s_dodwpattribsmap)
	{
		if (ke::StrCaseCmp(str, pair.second.data()) == 0)
		{
			return pair.first;
		}
	}

	return CDoDSWaypoint::DoDWPAttributes::DOD_WP_ATTRIBS_NONE;
}

void CDoDSWaypoint::DrawModText() const
{
	char attribs[256];
	attribs[0] = 0;
	char fmt[128];

	for (auto& pair : s_dodwpattribsmap)
	{
		if (HasDoDWaypointAttributes(pair.first))
		{
			ke::SafeSprintf(fmt, sizeof(fmt), "%s ", pair.second.data());
			ke::SafeStrcat(attribs, sizeof(attribs), fmt);
		}
	}

	NDebugOverlay::Text(GetOrigin() + Vector(0.0f, 0.0f, CWaypoint::WAYPOINT_MOD_TEXT_HEIGHT), false, NDEBUG_PERSIST_FOR_ONE_TICK, 
		"CP %i DoD Attribs: %s", m_CPIndex, attribs);
}
