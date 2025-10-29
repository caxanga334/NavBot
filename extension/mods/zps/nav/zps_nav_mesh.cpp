#include NAVBOT_PCH_FILE
#include "zps_nav_mesh.h"

extern NavAreaVector TheNavAreas;

CNavArea* CZPSNavMesh::CreateArea(void) const
{
	return new CZPSNavArea(GetNavPlace());
}
