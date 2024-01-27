#ifndef SMNAV_TF_NAV_MESH_H_
#define SMNAV_TF_NAV_MESH_H_
#pragma once

#include <navmesh/nav_mesh.h>

class CountdownTimer;

class CTFNavMesh : public CNavMesh
{
public:
	CTFNavMesh();
	virtual ~CTFNavMesh();

	virtual void OnRoundRestart(void) override;
	virtual CNavArea* CreateArea(void) const override;
	virtual unsigned int GetSubVersionNumber(void) const override;
	// Use nav mesh for climbing, players are limited when it comes to climbing
	virtual bool IsAuthoritative(void) const override { return true; }

	virtual void Update() override;

private:
	static constexpr auto NAV_SPAWNROOM_UPDATE_INTERVAL = 10.0f;
	CountdownTimer m_spawnroomupdatetimer;

	void UpdateDebugDraw();
};

inline unsigned int CTFNavMesh::GetSubVersionNumber(void) const
{
	/*
	* Sub version 
	* 1: Initial TF Nav Mesh implementation
	*/

	return 1;
}

#endif // !SMNAV_TF_NAV_MESH_H_

