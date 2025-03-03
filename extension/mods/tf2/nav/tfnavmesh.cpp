#include <cstring>

#include <extension.h>
#include <mods/tf2/teamfortress2mod.h>
#include <sdkports/sdk_timers.h>
#include <navmesh/nav_area.h>
#include "tfnavarea.h"
#include "tfnavmesh.h"
#include "tfnav_waypoint.h"

extern NavAreaVector TheNavAreas;

ConVar sm_tf_nav_show_path_attributes("sm_tf_nav_show_path_attributes", "0", FCVAR_GAMEDLL, "Shows TF Path Attributes");
ConVar sm_tf_nav_show_attributes("sm_tf_nav_show_attributes", "0", FCVAR_GAMEDLL, "Shows TF Attributes");
ConVar sm_tf_nav_show_mvm_attributes("sm_tf_nav_show_mvm_attributes", "0", FCVAR_GAMEDLL, "Shows TF MvM Attributes");

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
	navgenparams->climb_up_height = 68.0f;
	navgenparams->death_drop = 220.0f; // experimental

	ListenForGameEvent("mvm_wave_failed");
	ListenForGameEvent("mvm_wave_complete");
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
	// this method should be file for most maps
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
}

