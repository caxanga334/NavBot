#include NAVBOT_PCH_FILE
#include <utlbuffer.h>
#include <filesystem.h>
#include <mods/modhelpers.h>
#include "tfnavmesh.h"
#include "tfnavarea.h"

#undef max
#undef min
#undef clamp

void CTFNavArea::Save(std::fstream& filestream, uint32_t version)
{
	CNavArea::Save(filestream, version); // Save base first

	filestream.write(reinterpret_cast<char*>(&m_tfattributes), sizeof(int));
	filestream.write(reinterpret_cast<char*>(&m_tfpathattributes), sizeof(int));
	filestream.write(reinterpret_cast<char*>(&m_mvmattributes), sizeof(int));
}

NavErrorType CTFNavArea::Load(std::fstream& filestream, uint32_t version, uint32_t subVersion)
{
	auto base = CNavArea::Load(filestream, version, subVersion); // Load base first

	if (base != NAV_OK)
	{
		return base;
	}

	if (subVersion > 0)
	{
		filestream.read(reinterpret_cast<char*>(&m_tfattributes), sizeof(int));
		filestream.read(reinterpret_cast<char*>(&m_tfpathattributes), sizeof(int));
		filestream.read(reinterpret_cast<char*>(&m_mvmattributes), sizeof(int));

		if (!filestream.good())
		{
			return NAV_CORRUPT_DATA;
		}
	}

	return NAV_OK;
}

void CTFNavArea::UpdateBlocked(bool force, int teamID)
{
	CNavArea::UpdateBlocked(force, teamID);
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
	if (!HasTFPathAttributes(CTFNavArea::TFNavPathAttributes::TFNAV_PATH_DYNAMIC_SPAWNROOM))
	{
		return;
	}

	// default to Unassigned if none is found
	m_spawnroomteam = static_cast<int>(TeamFortress2::TFTeam::TFTeam_Unassigned);

	auto func = [this](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			bool disabled = false;
			entprops->GetEntPropBool(entity, Prop_Data, "m_bDisabled", disabled);

			if (disabled)
			{
				return true; // ignore disabled spawnroom entities.
			}

			if (this->IsCollidingWith(entity, navgenparams->human_crouch_height, false))
			{
				m_spawnroomteam = modhelpers->GetEntityTeamNumber(entity);
				return false; // stop search on the first found
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("func_respawnroom", func);
}

void CTFNavArea::Debug_ShowTFPathAttributes() const
{
	constexpr size_t TEXT_SIZE = 512U;
	char message[TEXT_SIZE]{};

	if (HasTFPathAttributes(TFNAV_PATH_NO_RED_TEAM))
	{
		ke::SafeStrcat(message, TEXT_SIZE, " NO_RED_TEAM");
		DrawFilled(153, 204, 255, 255, NDEBUG_PERSIST_FOR_ONE_TICK, true);
	}

	if (HasTFPathAttributes(TFNAV_PATH_NO_BLU_TEAM))
	{
		ke::SafeStrcat(message, TEXT_SIZE, " NO_BLU_TEAM");
		DrawFilled(255, 64, 64, 255, NDEBUG_PERSIST_FOR_ONE_TICK, true);
	}

	if (HasTFPathAttributes(TFNAV_PATH_NO_CARRIERS))
	{
		ke::SafeStrcat(message, TEXT_SIZE, " NO_FLAG_CARRIERS");
		DrawFilled(255, 69, 0, 255, NDEBUG_PERSIST_FOR_ONE_TICK, true);
	}

	if (HasTFPathAttributes(TFNAV_PATH_CARRIERS_AVOID))
	{
		ke::SafeStrcat(message, TEXT_SIZE, " FLAG_CARRIERS_AVOID");
	}

	if (HasTFPathAttributes(TFNAV_PATH_DYNAMIC_SPAWNROOM))
	{
		char buffer[64]{};
		ke::SafeSprintf(buffer, TEXT_SIZE, "DYNAMIC_SPAWNROOM -- TEAM = %i", m_spawnroomteam);
		ke::SafeStrcat(message, TEXT_SIZE, buffer);
		
		if (m_spawnroomteam == static_cast<int>(TeamFortress2::TFTeam::TFTeam_Blue))
		{
			DrawFilled(153, 204, 255, 255, NDEBUG_PERSIST_FOR_ONE_TICK, true);
		}
		else if (m_spawnroomteam == static_cast<int>(TeamFortress2::TFTeam::TFTeam_Red))
		{
			DrawFilled(255, 64, 64, 255, NDEBUG_PERSIST_FOR_ONE_TICK, true);
		}
	}

	NDebugOverlay::Text(GetCenter() + Vector(0.0f, 0.0f, 12.0f), message, true, NDEBUG_PERSIST_FOR_ONE_TICK);
}

void CTFNavArea::Debug_ShowTFAttributes() const
{
	constexpr size_t TEXT_SIZE = 512U;
	char message[TEXT_SIZE]{};

	if (HasTFAttributes(TFNAV_LIMIT_TO_REDTEAM))
	{
		ke::SafeStrcat(message, TEXT_SIZE, " LIMIT_TO_RED_TEAM");
		DrawFilled(153, 204, 255, 255, NDEBUG_PERSIST_FOR_ONE_TICK, true);
	}

	if (HasTFAttributes(TFNAV_LIMIT_TO_BLUTEAM))
	{
		ke::SafeStrcat(message, TEXT_SIZE, " LIMIT_TO_BLU_TEAM");
		DrawFilled(255, 64, 64, 255, NDEBUG_PERSIST_FOR_ONE_TICK, true);
	}

	NDebugOverlay::Text(GetCenter() + Vector(0.0f, 0.0f, 12.0f), message, true, NDEBUG_PERSIST_FOR_ONE_TICK);
}

void CTFNavArea::Debug_ShowMvMAttributes() const
{
	constexpr size_t TEXT_SIZE = 512U;
	char message[TEXT_SIZE]{};

	if (HasMVMAttributes(MVMNAV_FRONTLINES))
	{
		ke::SafeStrcat(message, TEXT_SIZE, " FRONTLINES");
		DrawFilled(0, 153, 0, 255, NDEBUG_PERSIST_FOR_ONE_TICK, true);
	}

	if (HasMVMAttributes(MVMNAV_UPGRADESTATION))
	{
		ke::SafeStrcat(message, TEXT_SIZE, " UPGRADE_STATION");
		DrawFilled(0, 60, 200, 255, NDEBUG_PERSIST_FOR_ONE_TICK, true);
	}

	NDebugOverlay::Text(GetCenter() + Vector(0.0f, 0.0f, 12.0f), message, true, NDEBUG_PERSIST_FOR_ONE_TICK);
}
