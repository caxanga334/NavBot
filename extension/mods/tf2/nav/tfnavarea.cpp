#include <limits>

#include <extension.h>
#include <utlbuffer.h>
#include <filesystem.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <sdkports/debugoverlay_shared.h>
#include "tfnavmesh.h"
#include "tfnavarea.h"

#undef max
#undef min
#undef clamp

void CTFNavArea::Save(CUtlBuffer& fileBuffer, unsigned int version, unsigned int portversion) const
{
	CNavArea::Save(fileBuffer, version, portversion); // Save base first

	fileBuffer.PutInt(m_tfattributes);
	fileBuffer.PutInt(m_tfpathattributes);
}

NavErrorType CTFNavArea::Load(CUtlBuffer& fileBuffer, unsigned int version, unsigned int portversion, unsigned int subVersion)
{
	auto base = CNavArea::Load(fileBuffer, version, portversion, subVersion); // Load base first

	if (base != NAV_OK)
	{
		return base;
	}

	if (subVersion > 0)
	{
		m_tfattributes = fileBuffer.GetInt();
		m_tfpathattributes = fileBuffer.GetInt();

		if (!fileBuffer.IsValid())
		{
			return NAV_CORRUPT_DATA;
		}
	}

	return NAV_OK;
}

bool CTFNavArea::IsBlocked(int teamID, bool ignoreNavBlockers) const
{
	if (HasTFPathAttributes(TFNAV_PATH_NO_RED_TEAM) && teamID == static_cast<int>(TeamFortress2::TFTeam::TFTeam_Red))
	{
		return true;
	}

	if (HasTFPathAttributes(TFNAV_PATH_NO_BLU_TEAM) && teamID == static_cast<int>(TeamFortress2::TFTeam::TFTeam_Blue))
	{
		return true;
	}

	if (HasTFPathAttributes(TFNAV_PATH_DYNAMIC_SPAWNROOM))
	{
		// nav area is tagged as spawnroom and doesn't belong to the given team
		if (m_spawnroomteam > TEAM_SPECTATOR && m_spawnroomteam != teamID)
		{
			return true;
		}
	}

	return CNavArea::IsBlocked(teamID, ignoreNavBlockers);
}

void CTFNavArea::UpdateDynamicSpawnRoom()
{
	int spawnroom = 0;
	const Vector& center = GetCenter();
	int nearest_team = 0; // store the team of the nearest func_respawnroom entity
	float smallest_dist = std::numeric_limits<float>::max();

	while ((spawnroom = UtilHelpers::FindEntityByClassname(spawnroom, "func_respawnroom")) != INVALID_EHANDLE_INDEX)
	{
		edict_t* edict = nullptr;
		
		if (!UtilHelpers::IndexToAThings(spawnroom, nullptr, &edict))
		{
			continue;
		}

		auto spawnroompos = UtilHelpers::getWorldSpaceCenter(edict);

		float current = center.DistTo(spawnroompos);

		if (current < smallest_dist)
		{
			smallest_dist = current;

			int lookup = 0;
			if (entprops->GetEntProp(spawnroom, Prop_Data, "m_iTeamNum", lookup))
			{
				nearest_team = lookup;
			}
		}
	}

	m_spawnroomteam = nearest_team;
}

void CTFNavArea::Debug_ShowTFPathAttributes() const
{
	constexpr auto TEXT_SIZE = 256;
	char message[TEXT_SIZE]{};

	if (HasTFPathAttributes(TFNAV_PATH_NO_RED_TEAM))
	{
		ke::SafeSprintf(message, TEXT_SIZE, "%s NO_RED_TEAM ", message);
		DrawFilled(153, 204, 255, 255, EXT_DEBUG_DRAW_TIME, true);
	}

	if (HasTFPathAttributes(TFNAV_PATH_NO_BLU_TEAM))
	{
		ke::SafeSprintf(message, TEXT_SIZE, "%s NO_BLU_TEAM ", message);
		DrawFilled(255, 64, 64, 255, EXT_DEBUG_DRAW_TIME, true);
	}

	if (HasTFPathAttributes(TFNAV_PATH_NO_CARRIERS))
	{
		ke::SafeSprintf(message, TEXT_SIZE, "%s NO_FLAG_CARRIERS ", message);
		DrawFilled(255, 69, 0, 255, EXT_DEBUG_DRAW_TIME, true);
	}

	if (HasTFPathAttributes(TFNAV_PATH_CARRIERS_AVOID))
	{
		ke::SafeSprintf(message, TEXT_SIZE, "%s FLAG_CARRIERS_AVOID ", message);
	}

	if (HasTFPathAttributes(TFNAV_PATH_DYNAMIC_SPAWNROOM))
	{
		ke::SafeSprintf(message, TEXT_SIZE, "%s DYNAMIC_SPAWNROOM -- TEAM = %i ", message, m_spawnroomteam);
		
		if (m_spawnroomteam == static_cast<int>(TeamFortress2::TFTeam::TFTeam_Blue))
		{
			DrawFilled(153, 204, 255, 255, EXT_DEBUG_DRAW_TIME, true);
		}
		else if (m_spawnroomteam == static_cast<int>(TeamFortress2::TFTeam::TFTeam_Red))
		{
			DrawFilled(255, 64, 64, 255, EXT_DEBUG_DRAW_TIME, true);
		}
	}

	NDebugOverlay::Text(GetCenter() + Vector(0.0f, 0.0f, 12.0f), message, true, EXT_DEBUG_DRAW_TIME);
}
