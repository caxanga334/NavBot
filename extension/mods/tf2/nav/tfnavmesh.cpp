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

bool CTFNavMesh::Save(void) const
{
	sm_nav_tf_show_path_attributes.SetValue(0); // disable rendering to avoid crashes during map change

	return CNavMesh::Save();
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

