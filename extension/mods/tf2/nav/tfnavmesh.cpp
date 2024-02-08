#include <extension.h>
#include <sdkports/sdk_timers.h>
#include <navmesh/nav_area.h>
#include "tfnavarea.h"
#include "tfnavmesh.h"

extern NavAreaVector TheNavAreas;

ConVar sm_nav_tf_show_path_attributes("sm_nav_tf_show_path_attributes", "0", FCVAR_GAMEDLL, "Shows TF Path Attributes");

CTFNavMesh::CTFNavMesh() : CNavMesh()
{
	m_spawnroomupdatetimer.Start(NAV_SPAWNROOM_UPDATE_INTERVAL);
}

CTFNavMesh::~CTFNavMesh()
{
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
	sm_nav_tf_show_path_attributes.SetValue(0); // disable rendering to avoid crashes during map change

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

void CTFNavMesh::UpdateDebugDraw()
{
	FOR_EACH_VEC(TheNavAreas, it)
	{
		CTFNavArea* area = static_cast<CTFNavArea*>(TheNavAreas[it]);
		
		if (sm_nav_tf_show_path_attributes.GetBool())
		{
			area->Debug_ShowTFPathAttributes();
		}
	}
}

