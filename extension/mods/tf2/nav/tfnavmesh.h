#ifndef SMNAV_TF_NAV_MESH_H_
#define SMNAV_TF_NAV_MESH_H_
#pragma once


#include <navmesh/nav_mesh.h>
#include "tfnavarea.h"
#include "tfnav_waypoint.h"

class CTFNavMesh : public CNavMesh
{
public:
	CTFNavMesh();
	~CTFNavMesh() override;

	void FireGameEvent(IGameEvent* event) override;
	void OnRoundRestart(void) override;
	CNavArea* CreateArea(void) const override;
	uint32_t GetSubVersionNumber(void) const override;
	// Use nav mesh for climbing, players are limited when it comes to climbing
	bool IsAuthoritative(void) const override { return true; }
	unsigned int GetGenerationTraceMask(void) const override;
	void OnNavMeshImportedPreSave() override;

	void Update() override;

	bool Save(void) override;

	void OnPreRCBot2WaypointImport(const CRCBot2WaypointLoader& loader) override;
	void OnRCBot2WaypointImported(const CRCBot2Waypoint& waypoint, const CRCBot2WaypointLoader& loader) override;

	// Returns a random nav area marked with the frontline attribute for MvM
	CTFNavArea* GetRandomFrontLineArea() const;
	// Returns a random spawn room exit area of a given team
	CTFNavArea* GetRandomSpawnRoomExitArea(int team) const;
	void AutoAddSpawnroomAttribute() const;
protected:
	void PostCustomAnalysis(void) override;

	// Creates a new waypoint instance
	std::shared_ptr<CWaypoint> CreateWaypoint() const override;
	CDoorNavBlocker* CreateDoorBlocker() const override;

private:
	static constexpr auto NAV_SPAWNROOM_UPDATE_INTERVAL = 10.0f;
	CountdownTimer m_spawnroomupdatetimer;
	std::unordered_map<int, int> m_rcbot2areatoindexmap;

	void UpdateDebugDraw();
};

inline uint32_t CTFNavMesh::GetSubVersionNumber(void) const
{
	/*
	* Sub version 
	* 1: Initial TF Nav Mesh implementation
	*/

	return 1;
}

inline CTFNavMesh* TheTFNavMesh()
{
	return reinterpret_cast<CTFNavMesh*>(TheNavMesh);
}

#endif // !SMNAV_TF_NAV_MESH_H_

