#include NAVBOT_PCH_FILE
#include <extension.h>
#include <ports/rcbot2_waypoint.h>
#include <mods/tf2/teamfortress2mod.h>
#include <sdkports/sdk_traces.h>
#include <navmesh/nav_area.h>
#include "tfnavarea.h"
#include "tfnavmesh.h"
#include "tfnav_waypoint.h"
#include "tfnav_door_blocker.h"

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
				auto functor = [this](CNavArea* area) {
					CTFNavArea* connectedArea = static_cast<CTFNavArea*>(area);

					if (!connectedArea->HasTFPathAttributes(CTFNavArea::TFNavPathAttributes::TFNAV_PATH_DYNAMIC_SPAWNROOM))
					{
						if (collected_areas.find(connectedArea->GetID()) == collected_areas.end())
						{
							collected_areas.insert(connectedArea->GetID());
							areas.push_back(connectedArea);
						}
					}
				};
				tfarea->ForEachConnectedArea(functor);
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

	/* No longer hard coded, moved to gamedata file
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
	*/

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
			if (std::strcmp(name, "mvm_wave_failed") == 0 || std::strcmp(name, "mvm_wave_complete") == 0 || 
				std::strcmp(name, "teamplay_round_start") == 0 || std::strcmp(name, "arena_round_start") == 0)
			{
				OnRoundRestart();
				PropagateOnRoundRestart();
				return;
			}
		}
	}

	CNavMesh::FireGameEvent(event);
}

void CTFNavMesh::RegisterModCommands()
{
	RegisterEditCommands();
	RegisterWaypointEditCommands();
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

void CTFNavMesh::OnNavMeshImportedPreSave()
{
	CNavMesh::OnNavMeshImportedPreSave();

	AutoAddSpawnroomAttribute();
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

void CTFNavMesh::OnPreRCBot2WaypointImport(const CRCBot2WaypointLoader& loader)
{
	m_rcbot2areatoindexmap.clear();
	m_rcbot2areatoindexmap.reserve(8);

	for (int i = 0; i < MAX_CONTROL_POINTS; i++)
	{
		CBaseEntity* pControlPoint = CTeamFortress2Mod::GetTF2Mod()->GetControlPointByIndex(i);

		if (pControlPoint)
		{
			Vector pos = UtilHelpers::getWorldSpaceCenter(pControlPoint);

			const CRCBot2Waypoint* wpt = loader.GetNearestFlaggedWaypoint(pos, CRCBot2Waypoint::W_FL_CAPPOINT, 1024.0f);

			if (wpt)
			{
				int area = wpt->GetAreaNumber();
				m_rcbot2areatoindexmap[area] = i;
#ifdef EXT_DEBUG
				META_CONPRINTF("[TFNavMesh] Mapped RCBot2 area number %i to control point index %i!\n", area, i);
#endif // EXT_DEBUG
			}
		}
	}
}

void CTFNavMesh::OnRCBot2WaypointImported(const CRCBot2Waypoint& waypoint, const CRCBot2WaypointLoader& loader)
{
	using namespace TeamFortress2;

	if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_UNREACHABLE))
		return;

	auto gm = CTeamFortress2Mod::GetTF2Mod()->GetCurrentGameMode();
	CTFWaypoint* wpt = nullptr;

	bool gamemodeusesArea = (gm == GameModeType::GM_CP || gm == GameModeType::GM_ADCP || gm == GameModeType::GM_PL || gm == GameModeType::GM_PL_RACE);
	bool setAngle = false;
	bool assignControlPoint = false;

	if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_SNIPER))
	{
		auto result = TheNavMesh->AddWaypoint(waypoint.GetOrigin());
		wpt = static_cast<CTFWaypoint*>(result.value().get());
		
		wpt->SetTFHint(CTFWaypoint::TFHINT_SNIPER);

		setAngle = true;
		assignControlPoint = true;
	}
	else if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_SENTRY))
	{
		auto result = TheNavMesh->AddWaypoint(waypoint.GetOrigin());
		wpt = static_cast<CTFWaypoint*>(result.value().get());

		wpt->SetTFHint(CTFWaypoint::TFHINT_SENTRYGUN);

		setAngle = true;
		assignControlPoint = true;
	}
	else if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_TELE_EXIT))
	{
		auto result = TheNavMesh->AddWaypoint(waypoint.GetOrigin());
		wpt = static_cast<CTFWaypoint*>(result.value().get());

		wpt->SetTFHint(CTFWaypoint::TFHINT_TELE_EXIT);

		setAngle = true;
		assignControlPoint = true;
	}
	else if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_TELE_ENTRANCE))
	{
		auto result = TheNavMesh->AddWaypoint(waypoint.GetOrigin());
		wpt = static_cast<CTFWaypoint*>(result.value().get());

		wpt->SetTFHint(CTFWaypoint::TFHINT_TELE_ENTRANCE);

		setAngle = true;
		assignControlPoint = true;
	}
	else if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_DEFEND))
	{
		auto result = TheNavMesh->AddWaypoint(waypoint.GetOrigin());
		wpt = static_cast<CTFWaypoint*>(result.value().get());

		wpt->SetFlags(CWaypoint::BaseFlags::BASEFLAGS_DEFEND);

		setAngle = true;
		assignControlPoint = true;
	}
	else if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_ROUTE))
	{
		auto result = TheNavMesh->AddWaypoint(waypoint.GetOrigin());
		wpt = static_cast<CTFWaypoint*>(result.value().get());

		// rcbot2 route and navbot roam flags are different things so this is a bit experimental
		wpt->SetFlags(CWaypoint::BaseFlags::BASEFLAGS_ROAM);

		setAngle = true;
		assignControlPoint = true;
	}

	if (!wpt)
	{
		return;
	}

	wpt->SetRadius(waypoint.GetRadius());

	if (setAngle)
	{
		const float yaw = AngleNormalizePositive(static_cast<float>(waypoint.GetAimYaw()));
		QAngle angle{ 0.0f, yaw, 0.0f };
		wpt->AddAngle(angle);
	}

	if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_CROUCH))
	{
		wpt->SetFlags(CWaypoint::BaseFlags::BASEFLAGS_CROUCH);
	}

	if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_NORED))
	{
		wpt->SetTeam(static_cast<int>(TFTeam::TFTeam_Blue));
	}
	else if (waypoint.HasFlags(CRCBot2Waypoint::W_FL_NOBLU))
	{
		wpt->SetTeam(static_cast<int>(TFTeam::TFTeam_Red));
	}

	if (assignControlPoint && gamemodeusesArea)
	{
		int area = waypoint.GetAreaNumber();

		auto it = m_rcbot2areatoindexmap.find(area);

		if (it == m_rcbot2areatoindexmap.end())
		{
#ifdef EXT_DEBUG
			if (area > 0)
			{
				META_CONPRINTF("[RCBot2 Import] Waypoint area %i not mapped to a control point index! \n", area);
			}
#endif // EXT_DEBUG
			wpt->SetControlPointIndex(CTFWaypoint::NO_CONTROL_POINT);
		}
		else
		{
			wpt->SetControlPointIndex(it->second);
		}
	}
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

void CTFNavMesh::AutoAddSpawnroomAttribute() const
{
	int areas = 0;

	auto functor = [&areas](CNavArea* baseArea) {
		CTFNavArea* area = static_cast<CTFNavArea*>(baseArea);
		Vector center = area->GetCenter();
		center.z += 24.0f;
		bool inside = false;

		auto functor2 = [&inside, &center](int index, edict_t* edict, CBaseEntity* entity) {
			if (edict != nullptr && edict->GetCollideable() != nullptr)
			{
				if (trace::pointwithin(edict->GetCollideable(), center))
				{
					inside = true;
					return false; // exit search
				}
			}

			return true; // keep searching for entities
			};

		UtilHelpers::ForEachEntityOfClassname("func_respawnroom", functor2);

		if (inside)
		{
			areas++;
			area->SetTFPathAttributes(CTFNavArea::TFNAV_PATH_DYNAMIC_SPAWNROOM);
		}

		return true;
	};

	CNavMesh::ForAllAreas(functor);

	Msg("Added spawn room attribute to %i nav areas!\n", areas);
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

CDoorNavBlocker* CTFNavMesh::CreateDoorBlocker() const
{
	return new CTFDoorNavBlocker;
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

