#ifndef __NAVBOT_MOD_CSS_NAV_MESH_H_
#define __NAVBOT_MOD_CSS_NAV_MESH_H_

#include <navmesh/nav_mesh.h>
#include "css_nav_area.h"

class CCSSNavMesh : public CNavMesh
{
public:
	CCSSNavMesh();

	CNavArea* CreateArea(void) const override;
	uint32_t GetSubVersionNumber(void) const override;

private:

};

inline CCSSNavMesh* TheCSSNavMesh()
{
	return static_cast<CCSSNavMesh*>(TheNavMesh);
}


#endif // !__NAVBOT_MOD_CSS_NAV_MESH_H_
