#include NAVBOT_PCH_FILE
#include "css_nav_mesh.h"

CCSSNavMesh::CCSSNavMesh() :
	CNavMesh()
{
}

CNavArea* CCSSNavMesh::CreateArea(void) const
{
	return new CCSSNavArea(GetNavPlace());
}

uint32_t CCSSNavMesh::GetSubVersionNumber(void) const
{
	return 0;
}

