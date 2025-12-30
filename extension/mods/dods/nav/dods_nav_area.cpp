#include NAVBOT_PCH_FILE
#include <extension.h>
#include <mods/dods/dayofdefeatsourcemod.h>
#include <mods/dods/dodslib.h>
#include "dods_nav_mesh.h"
#include "dods_nav_area.h"

CDoDSNavArea::CDoDSNavArea(unsigned int place) :
	CNavArea(place)
{
	m_dodAttributes = 0;
	m_bombTarget = nullptr;
	m_bombed = false;
	m_bombTeam = NAV_TEAM_ANY;
}

CDoDSNavArea::~CDoDSNavArea()
{
}

void CDoDSNavArea::OnUpdate()
{
	CNavArea::OnUpdate();

	if (HasBombRelatedAttributes() && !m_bombed)
	{
		CBaseEntity* bomb = GetAssignedBombTarget();

		if (bomb)
		{
			int state = static_cast<int>(dayofdefeatsource::DoDBombTargetState::BOMB_TARGET_INACTIVE);
			entprops->GetEntProp(bomb, Prop_Send, "m_iState", state);
			m_bombed = (state == static_cast<int>(dayofdefeatsource::DoDBombTargetState::BOMB_TARGET_INACTIVE));
		}
		else
		{
			m_bombed = true; // NULL bomb target entity
		}
	}
}

void CDoDSNavArea::OnRoundRestart(void)
{
	CNavArea::OnRoundRestart();

	if (HasBombRelatedAttributes())
	{
		FindAndAssignNearestBombTarget();
	}
}

void CDoDSNavArea::Save(std::fstream& filestream, uint32_t version)
{
	CNavArea::Save(filestream, version);

	filestream.write(reinterpret_cast<char*>(&m_dodAttributes), sizeof(int));
}

NavErrorType CDoDSNavArea::Load(std::fstream& filestream, uint32_t version, uint32_t subVersion)
{
	NavErrorType code = CNavArea::Load(filestream, version, subVersion);

	if (subVersion > 0U)
	{
		filestream.read(reinterpret_cast<char*>(&m_dodAttributes), sizeof(int));
	}

	return code;
}

NavErrorType CDoDSNavArea::PostLoad(void)
{
	NavErrorType code = CNavArea::PostLoad();

	if (HasDoDAttributes(CDoDSNavArea::DoDNavAttributes::DODNAV_DEPRECATED1))
	{
		smutils->LogError(myself, "Removing deprecated DoD attributes from Nav area %i at %s!", GetID(), UtilHelpers::textformat::FormatVector(GetCenter()));
		ClearDoDAttributes(CDoDSNavArea::DoDNavAttributes::DODNAV_DEPRECATED1);
	}

	if (HasDoDAttributes(CDoDSNavArea::DoDNavAttributes::DODNAV_DEPRECATED2))
	{
		smutils->LogError(myself, "Removing deprecated DoD attributes from Nav area %i at %s!", GetID(), UtilHelpers::textformat::FormatVector(GetCenter()));
		ClearDoDAttributes(CDoDSNavArea::DoDNavAttributes::DODNAV_DEPRECATED2);
	}

	return code;
}

bool CDoDSNavArea::IsBlocked(int teamID, bool ignoreNavBlockers) const
{
	if (teamID == static_cast<int>(dayofdefeatsource::DoDTeam::DODTEAM_ALLIES))
	{
		if (HasDoDAttributes(DODNAV_NO_ALLIES))
		{
			return true;
		}
	}

	if (teamID == static_cast<int>(dayofdefeatsource::DoDTeam::DODTEAM_AXIS))
	{
		if (HasDoDAttributes(DODNAV_NO_AXIS))
		{
			return true;
		}
	}

	return CNavArea::IsBlocked(teamID, ignoreNavBlockers);
}

const bool CDoDSNavArea::CanPlantBomb() const
{
	CBaseEntity* bomb = GetAssignedBombTarget();

	if (!bomb)
	{
		return false; // entity may have been deleted, assume it's open.
	}

	int state = -1;
	entprops->GetEntProp(bomb, Prop_Send, "m_iState", state);
	return state == static_cast<int>(dayofdefeatsource::DoDBombTargetState::BOMB_TARGET_ACTIVE);
}

void CDoDSNavArea::ShowAreaInfo() const
{
	if (m_dodAttributes != 0)
	{
		char dodattribs[256];
		dodattribs[0] = '\0';

		ke::SafeStrcpy(dodattribs, sizeof(dodattribs), "DoD Attributes: ");

		if (HasDoDAttributes(DODNAV_NO_ALLIES)) { ke::SafeStrcat(dodattribs, sizeof(dodattribs), "NO_ALLIES"); }
		if (HasDoDAttributes(DODNAV_NO_AXIS)) { ke::SafeStrcat(dodattribs, sizeof(dodattribs), "NO_AXIS"); }
		// if (HasDoDAttributes(DODNAV_BLOCKED_UNTIL_BOMBED)) { ke::SafeStrcat(dodattribs, sizeof(dodattribs), "BLOCKED_UNTIL_BOMBED"); }
		// if (HasDoDAttributes(DODNAV_BLOCKED_WITHOUT_BOMBS)) { ke::SafeStrcat(dodattribs, sizeof(dodattribs), "BLOCKED_WITHOUT_BOMBS"); }
		if (HasDoDAttributes(DODNAV_BOMBS_TO_OPEN)) { ke::SafeStrcat(dodattribs, sizeof(dodattribs), "BOMBS_TO_OPEN"); }
		if (HasDoDAttributes(DODNAV_REQUIRES_PRONE)) { ke::SafeStrcat(dodattribs, sizeof(dodattribs), "REQUIRES_PRONE"); }

		debugoverlay->AddScreenTextOverlay(0.37f, 0.6f, NDEBUG_PERSIST_FOR_ONE_TICK, 255, 255, 0, 255, dodattribs);

		if (HasBombRelatedAttributes())
		{
			CBaseEntity* bomb = GetAssignedBombTarget();

			if (bomb)
			{
				NDebugOverlay::EntityBounds(bomb, 255, 255, 0, 128, NDEBUG_PERSIST_FOR_ONE_TICK);
				NDebugOverlay::Text(UtilHelpers::getWorldSpaceCenter(bomb), true, NDEBUG_PERSIST_FOR_ONE_TICK, "AREA #%u BOMB", GetID());
			}
		}
	}
}

void CDoDSNavArea::FindAndAssignNearestBombTarget()
{
	m_bombTarget = nullptr;
	const Vector& center = GetCenter();
	CBaseEntity* bomb = UtilHelpers::GetNearestEntityOfClassname(center, "dod_bomb_target");

	if (!bomb)
	{
		smutils->LogError(myself, "CDoDSNavArea #%u at %g %g %g has bomb attributes but could not find any dod_bomb_target entity!", GetID(), center.x, center.y, center.z);
		return;
	}

	m_bombTarget = bomb;
	m_bombed = false;

	int team = NAV_TEAM_ANY;
	entprops->GetEntProp(bomb, Prop_Send, "m_iBombingTeam", team);

	if (team == static_cast<int>(dayofdefeatsource::DoDTeam::DODTEAM_ALLIES) || team == static_cast<int>(dayofdefeatsource::DoDTeam::DODTEAM_AXIS))
	{
		m_bombTeam = team;
	}
	else
	{
		m_bombTeam = NAV_TEAM_ANY;
	}
}
