#ifndef __NAVBOT_DODS_NAV_MESH_H_
#define __NAVBOT_DODS_NAV_MESH_H_

#include <navmesh/nav_mesh.h>

class CDODSNavArea;

class CDODSNavMesh : public CNavMesh
{
public:
	CDODSNavMesh();
	~CDODSNavMesh() override;

	unsigned int GetGenerationTraceMask(void) const override;
	CNavArea* CreateArea(void) const override;

private:

};

#endif // !__NAVBOT_DODS_NAV_MESH_H_