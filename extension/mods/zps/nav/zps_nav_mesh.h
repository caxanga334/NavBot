#ifndef __NAVBOT_ZPS_NAV_MESH_H_
#define __NAVBOT_ZPS_NAV_MESH_H_

#include <navmesh/nav_mesh.h>
#include "zps_nav_area.h"
#include "zps_nav_door_blocker.h"

class CZPSNavMesh : public CNavMesh
{
public:

	CNavArea* CreateArea(void) const override;

protected:
	void RegisterModCommands() override;
	CDoorNavBlocker* CreateDoorBlocker() const override { return new CZPSDoorBlocker; }

private:

};


#endif // !__NAVBOT_ZPS_NAV_MESH_H_
