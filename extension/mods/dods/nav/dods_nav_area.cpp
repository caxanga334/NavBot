#include NAVBOT_PCH_FILE
#include <extension.h>
#include <mods/dods/dayofdefeatsourcemod.h>
#include <mods/dods/dodslib.h>
#include "dods_nav_mesh.h"
#include "dods_nav_area.h"

CDODSNavArea::CDODSNavArea(unsigned int place) :
	CNavArea(place)
{
	m_dodAttributes = 0;
	m_bombTarget = nullptr;
}

CDODSNavArea::~CDODSNavArea()
{
}

void CDODSNavArea::OnRoundRestart(void)
{
	CNavArea::OnRoundRestart();

	if (HasBombRelatedAttributes())
	{
		FindAndAssignNearestBombTarget();
	}
}

void CDODSNavArea::Save(std::fstream& filestream, uint32_t version)
{
	CNavArea::Save(filestream, version);

	filestream.write(reinterpret_cast<char*>(&m_dodAttributes), sizeof(int));
}

NavErrorType CDODSNavArea::Load(std::fstream& filestream, uint32_t version, uint32_t subVersion)
{
	NavErrorType code = CNavArea::Load(filestream, version, subVersion);

	if (subVersion > 0U)
	{
		filestream.read(reinterpret_cast<char*>(&m_dodAttributes), sizeof(int));
	}

	return code;
}

NavErrorType CDODSNavArea::PostLoad(void)
{
	NavErrorType code = CNavArea::PostLoad();

	return code;
}

bool CDODSNavArea::IsBlocked(int teamID, bool ignoreNavBlockers) const
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

	if (HasDoDAttributes(DODNAV_BLOCKED_UNTIL_BOMBED))
	{
		if (!WasBombed())
		{
			return true;
		}
	}

	return CNavArea::IsBlocked(teamID, ignoreNavBlockers);
}

const bool CDODSNavArea::WasBombed() const
{
	CBaseEntity* bomb = GetAssignedBombTarget();

	if (!bomb)
	{
		return true; // entity may have been deleted, assume it's open.
	}

	int state = -1;
	entprops->GetEntProp(bomb, Prop_Send, "m_iState", state);

	if (state == static_cast<int>(dayofdefeatsource::DoDBombTargetState::BOMB_TARGET_INACTIVE))
	{
		return true;
	}

	return false;
}

const bool CDODSNavArea::CanPlantBomb() const
{
	CBaseEntity* bomb = GetAssignedBombTarget();

	if (!bomb)
	{
		return false; // entity may have been deleted, assume it's open.
	}

	int state = -1;
	entprops->GetEntProp(bomb, Prop_Send, "m_iState", state);

	if (state == static_cast<int>(dayofdefeatsource::DoDBombTargetState::BOMB_TARGET_ACTIVE))
	{
		return true;
	}

	return false;
}

void CDODSNavArea::ShowAreaInfo() const
{
	if (m_dodAttributes != 0)
	{
		char dodattribs[256];
		dodattribs[0] = '\0';

		ke::SafeStrcpy(dodattribs, sizeof(dodattribs), "DoD Attributes: ");

		if (HasDoDAttributes(DODNAV_NO_ALLIES)) { ke::SafeStrcat(dodattribs, sizeof(dodattribs), "NO_ALLIES"); }
		if (HasDoDAttributes(DODNAV_NO_AXIS)) { ke::SafeStrcat(dodattribs, sizeof(dodattribs), "NO_AXIS"); }
		if (HasDoDAttributes(DODNAV_BLOCKED_UNTIL_BOMBED)) { ke::SafeStrcat(dodattribs, sizeof(dodattribs), "BLOCKED_UNTIL_BOMBED"); }
		if (HasDoDAttributes(DODNAV_BLOCKED_WITHOUT_BOMBS)) { ke::SafeStrcat(dodattribs, sizeof(dodattribs), "BLOCKED_WITHOUT_BOMBS"); }
		if (HasDoDAttributes(DODNAV_PLANT_BOMB)) { ke::SafeStrcat(dodattribs, sizeof(dodattribs), "PLANT_BOMB"); }
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

void CDODSNavArea::FindAndAssignNearestBombTarget()
{
	m_bombTarget = nullptr;
	const Vector& center = GetCenter();
	CBaseEntity* bomb = UtilHelpers::GetNearestEntityOfClassname(center, "dod_bomb_target");

	if (!bomb)
	{
		smutils->LogError(myself, "CDODSNavArea #%u at %g %g %g has bomb attributes but could not find any dod_bomb_target entity!", GetID(), center.x, center.y, center.z);
		return;
	}

	m_bombTarget = bomb;
}
