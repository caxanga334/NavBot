#include <cstring>
#include <unordered_set>
#include <vector>

#include <extension.h>
#include <mods/tf2/teamfortress2mod.h>
#include <sdkports/sdk_timers.h>
#include <navmesh/nav_area.h>
#include "tfnavarea.h"
#include "tfnavmesh.h"
#include "tfnav_waypoint.h"

extern NavAreaVector TheNavAreas;

#if SOURCE_ENGINE == SE_TF2
ConVar sm_tf_nav_show_path_attributes("sm_tf_nav_show_path_attributes", "0", FCVAR_GAMEDLL, "Shows TF Path Attributes");
ConVar sm_tf_nav_show_attributes("sm_tf_nav_show_attributes", "0", FCVAR_GAMEDLL, "Shows TF Attributes");
ConVar sm_tf_nav_show_mvm_attributes("sm_tf_nav_show_mvm_attributes", "0", FCVAR_GAMEDLL, "Shows TF MvM Attributes");
#endif // SOURCE_ENGINE == SE_TF2

namespace tfnavmeshutils
{
	class TFNavMeshCollectSpawnRoomExits
	{
	public:
		TFNavMeshCollectSpawnRoomExits(int teamNum)
		{
			team_index = teamNum;
			collected_areas.reserve(256);
			areas.reserve(256);
		}

		bool operator()(CNavArea* baseArea)
		{
			CTFNavArea* tfarea = static_cast<CTFNavArea*>(baseArea);

			if (tfarea->HasTFPathAttributes(CTFNavArea::TFNavPathAttributes::TFNAV_PATH_DYNAMIC_SPAWNROOM) &&
				tfarea->GetSpawnRoomTeam() == team_index)
			{
				// current area is a spawn room area, loop connected areas and add any areas that does not have the spawn room attribute to the list
				tfarea->ForEachConnectedArea([this](CNavArea* area) {
					CTFNavArea* connectedArea = static_cast<CTFNavArea*>(area);

					if (!connectedArea->HasTFPathAttributes(CTFNavArea::TFNavPathAttributes::TFNAV_PATH_DYNAMIC_SPAWNROOM))
					{
						if (collected_areas.find(connectedArea->GetID()) == collected_areas.end())
						{
							collected_areas.insert(connectedArea->GetID());
							areas.push_back(connectedArea);
						}
					}
				});
			}

			return true;
		}

		void GetAreas(std::vector<CTFNavArea*>& vec)
		{
			vec.swap(areas);
		}

	private:
		int team_index;
		std::unordered_set<unsigned int> collected_areas;
		std::vector<CTFNavArea*> areas;
	};
}

CTFNavMesh::CTFNavMesh() : CNavMesh()
{
	m_spawnroomupdatetimer.Start(NAV_SPAWNROOM_UPDATE_INTERVAL);

	AddWalkableEntity("item_ammopack_full");
	AddWalkableEntity("item_ammopack_medium");
	AddWalkableEntity("item_ammopack_small");
	AddWalkableEntity("item_healthkit_full");
	AddWalkableEntity("item_healthkit_medium");
	AddWalkableEntity("item_healthkit_small");
	AddWalkableEntity("item_teamflag");
	AddWalkableEntity("func_capturezone", true);
	AddWalkableEntity("func_regenerate", true);
	AddWalkableEntity("team_control_point");
	AddWalkableEntity("trigger_capture_area", true);
	AddWalkableEntity("trigger_timer_door", true);
	AddWalkableEntity("info_powerup_spawn");

	navgenparams->half_human_width = 24.0f;
	navgenparams->half_human_height = 41.0f;
	navgenparams->human_height = 82.0f;
	navgenparams->human_crouch_height = 62.0f;
	navgenparams->human_eye_height = 74.0f;
	navgenparams->human_crouch_eye_height = 44.0f;
	navgenparams->jump_height = 49.0f;
	navgenparams->jump_crouch_height = 68.0f;
	navgenparams->climb_up_height = 128.0f;
	navgenparams->death_drop = 300.0f; // experimental

	ListenForGameEvent("mvm_wave_failed");
	ListenForGameEvent("mvm_wave_complete");
	ListenForGameEvent("teamplay_round_start");
	ListenForGameEvent("arena_round_start");
}

CTFNavMesh::~CTFNavMesh()
{
}

void CTFNavMesh::FireGameEvent(IGameEvent* event)
{
	if (event != nullptr)
	{
		const char* name = event->GetName();

		if (name)
		{
			if (std::strcmp(name, "mvm_wave_failed") == 0 || std::strcmp(name, "mvm_wave_complete") == 0)
			{
				OnRoundRestart();
				PropagateOnRoundRestart();
				return;
			}
			else if (std::strcmp(name, "teamplay_round_start") == 0 || std::strcmp(name, "arena_round_start") == 0)
			{
				OnRoundRestart();
				PropagateOnRoundRestart();
			}
		}
	}

	CNavMesh::FireGameEvent(event);
}

void CTFNavMesh::OnRoundRestart(void)
{
	CNavMesh::OnRoundRestart();
	m_spawnroomupdatetimer.Start(NAV_SPAWNROOM_UPDATE_INTERVAL);
}

CNavArea* CTFNavMesh::CreateArea(void) const
{
	return new CTFNavArea(GetNavPlace());
}

unsigned int CTFNavMesh::GetGenerationTraceMask(void) const
{
	return MASK_PLAYERSOLID_BRUSHONLY;
}

void CTFNavMesh::Update()
{
	CNavMesh::Update();
	UpdateDebugDraw();

	if (m_spawnroomupdatetimer.IsElapsed())
	{
		m_spawnroomupdatetimer.Start(NAV_SPAWNROOM_UPDATE_INTERVAL);

		FOR_EACH_VEC(TheNavAreas, it)
		{
			CTFNavArea* area = static_cast<CTFNavArea*>(TheNavAreas[it]);
			area->UpdateDynamicSpawnRoom();
		}
	}
}

bool CTFNavMesh::Save(void)
{
	return CNavMesh::Save();
}

std::string CTFNavMesh::GetMapFileName() const
{
	// TF2 workshop names follows this format: workshop/ctf_harbine.ugc3067683041
	// this method should be fine for most maps
	std::string mapname(gamehelpers->GetCurrentMap());

	auto n1 = mapname.find("workshop/");
	auto n2 = mapname.find(".ugc");

	if (n1 == std::string::npos)
	{
		return mapname; // not a workshop map
	}

	constexpr size_t WORKSHOP_STR_SIZE = 9U;
	constexpr size_t UGC_STR_SIZE = 4U;
	auto count = n2 - (n1 + WORKSHOP_STR_SIZE);
	auto sub1 = mapname.substr(n1 + WORKSHOP_STR_SIZE, count);
	auto sub2 = mapname.substr(n2 + UGC_STR_SIZE);
	std::string finalname = sub1 + "_ugc" + sub2; // becomes something like this: ctf_harbine_ugc3067683041
	return finalname;
}

CTFNavArea* CTFNavMesh::GetRandomFrontLineArea() const
{
	std::vector<CTFNavArea*> frontlineAreas;

	auto collectFrontLineAreas = [&frontlineAreas](CNavArea* area) {

		CTFNavArea* tfarea = static_cast<CTFNavArea*>(area);

		if (tfarea->HasMVMAttributes(CTFNavArea::MvMNavAttributes::MVMNAV_FRONTLINES))
		{
			frontlineAreas.push_back(tfarea);
		}

		return true;
	};

	CNavMesh::ForAllAreas<decltype(collectFrontLineAreas)>(collectFrontLineAreas);

	if (!frontlineAreas.empty())
	{
		return librandom::utils::GetRandomElementFromVector(frontlineAreas);
	}

	return nullptr;
}

CTFNavArea* CTFNavMesh::GetRandomSpawnRoomExitArea(int team) const
{

	tfnavmeshutils::TFNavMeshCollectSpawnRoomExits functor(team);

	CNavMesh::ForAllAreas(functor);

	std::vector<CTFNavArea*> spawnexitareas;
	functor.GetAreas(spawnexitareas);

	if (spawnexitareas.empty())
	{
		return nullptr;
	}

	return librandom::utils::GetRandomElementFromVector<CTFNavArea*>(spawnexitareas);
}

void CTFNavMesh::PostCustomAnalysis(void)
{
	CTeamFortress2Mod* mod = CTeamFortress2Mod::GetTF2Mod();

	bool hasfrontline = false;

	FOR_EACH_VEC(TheNavAreas, it)
	{
		CTFNavArea* area = static_cast<CTFNavArea*>(TheNavAreas[it]);

		if (mod->GetCurrentGameMode() == TeamFortress2::GameModeType::GM_MVM)
		{
			if (!hasfrontline && area->HasMVMAttributes(CTFNavArea::MVMNAV_FRONTLINES))
			{
				hasfrontline = true;
			}
		}
	}

	if (!hasfrontline && mod->GetCurrentGameMode() == TeamFortress2::GameModeType::GM_MVM)
	{
		smutils->LogError(myself, "Mann vs Machine navmesh without \"Frontlines\" areas!");
	}
}

std::shared_ptr<CWaypoint> CTFNavMesh::CreateWaypoint() const
{
	return std::make_shared<CTFWaypoint>();
}

void CTFNavMesh::UpdateDebugDraw()
{
	edict_t* ent = gamehelpers->EdictOfIndex(1);

	if (ent == nullptr || ent->GetIServerEntity() == nullptr)
		return;

#if SOURCE_ENGINE == SE_TF2
	FOR_EACH_VEC(TheNavAreas, it)
	{
		CTFNavArea* area = static_cast<CTFNavArea*>(TheNavAreas[it]);
		
		if (sm_tf_nav_show_path_attributes.GetBool())
		{
			area->Debug_ShowTFPathAttributes();
		}

		if (sm_tf_nav_show_attributes.GetBool())
		{
			area->Debug_ShowTFAttributes();
		}

		if (sm_tf_nav_show_mvm_attributes.GetBool())
		{
			area->Debug_ShowMvMAttributes();
		}
	}
#endif // SOURCE_ENGINE == SE_TF2
}

