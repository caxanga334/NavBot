#ifndef __NAVBOT_DODS_NAV_MESH_H_
#define __NAVBOT_DODS_NAV_MESH_H_

#include <navmesh/nav_mesh.h>

class CDoDSNavArea;

class CDoDSNavMesh : public CNavMesh
{
public:
	CDoDSNavMesh();
	~CDoDSNavMesh() override;
	
	/* DoD Sub Version History:
	* 1: Initial implementation of the DoD specific nav mesh
	*/

	static constexpr uint32_t DoDNavSubVersion = 1U; // current dod nav mesh sub version

	void RegisterModCommands() override;
	unsigned int GetGenerationTraceMask(void) const override;
	CNavArea* CreateArea(void) const override;

	uint32_t GetSubVersionNumber(void) const override { return DoDNavSubVersion; }

private:

	void RegisterEditCommands();
};

#endif // !__NAVBOT_DODS_NAV_MESH_H_