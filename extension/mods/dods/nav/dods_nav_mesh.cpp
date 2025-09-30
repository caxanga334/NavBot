#include NAVBOT_PCH_FILE
#include <extension.h>
#include "dods_nav_area.h"
#include "dods_nav_mesh.h"

CDODSNavMesh::CDODSNavMesh() :
	CNavMesh()
{
}

CDODSNavMesh::~CDODSNavMesh()
{
}

unsigned int CDODSNavMesh::GetGenerationTraceMask(void) const
{
	return MASK_PLAYERSOLID_BRUSHONLY;
}

CNavArea* CDODSNavMesh::CreateArea(void) const
{
	return new CDODSNavArea(GetNavPlace());
}
