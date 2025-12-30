#ifndef __NAVBOT_ZPS_NAV_MESH_H_
#define __NAVBOT_ZPS_NAV_MESH_H_

#include <navmesh/nav_mesh.h>
#include "zps_nav_area.h"

class CZPSNavMesh : public CNavMesh
{
public:

	CNavArea* CreateArea(void) const override;

protected:
	void RegisterModCommands() override;

private:

};


#endif // !__NAVBOT_ZPS_NAV_MESH_H_
