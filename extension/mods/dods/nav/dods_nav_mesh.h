#ifndef __NAVBOT_DODS_NAV_MESH_H_
#define __NAVBOT_DODS_NAV_MESH_H_

#include <navmesh/nav_mesh.h>

class CDoDSNavArea;
class CDoDSWaypoint;

class CDoDSNavMesh : public CNavMesh
{
public:
	CDoDSNavMesh();
	~CDoDSNavMesh() override;
	
	/* DoD Sub Version History:
	* 1: Initial implementation of the DoD specific nav mesh
	* 2: Due to a bug, no data changes but version was already bumped.
	* 3: Added DoD specific nav waypoint.
	*/

	static constexpr uint32_t DoDNavSubVersion = 3U; // current dod nav mesh sub version

	void RegisterModCommands() override;
	unsigned int GetGenerationTraceMask(void) const override;
	CNavArea* CreateArea(void) const override;
	uint32_t GetSubVersionNumber(void) const override { return DoDNavSubVersion; }
	void OnPreRCBot2WaypointImport(const CRCBot2WaypointLoader& loader) override;
	void OnRCBot2WaypointImported(const CRCBot2Waypoint& waypoint, const CRCBot2WaypointLoader& loader) override;
protected:
	std::shared_ptr<CWaypoint> CreateWaypoint() const override;

private:

	void RegisterEditCommands();
};

#endif // !__NAVBOT_DODS_NAV_MESH_H_