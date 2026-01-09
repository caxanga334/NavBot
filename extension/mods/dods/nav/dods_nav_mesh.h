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
	* 2: Added DoD specific nav waypoint
	*/

	static constexpr uint32_t DoDNavSubVersion = 2U; // current dod nav mesh sub version

	void RegisterModCommands() override;
	unsigned int GetGenerationTraceMask(void) const override;
	CNavArea* CreateArea(void) const override;

	uint32_t GetSubVersionNumber(void) const override { return DoDNavSubVersion; }

private:

	void RegisterEditCommands();
};

#endif // !__NAVBOT_DODS_NAV_MESH_H_