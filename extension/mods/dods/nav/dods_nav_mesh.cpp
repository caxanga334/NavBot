#include NAVBOT_PCH_FILE
#include <extension.h>
#include "dods_nav_area.h"
#include "dods_nav_mesh.h"

CDoDSNavMesh::CDoDSNavMesh() :
	CNavMesh()
{
}

CDoDSNavMesh::~CDoDSNavMesh()
{
}

void CDoDSNavMesh::RegisterModCommands()
{
	RegisterEditCommands();
}

unsigned int CDoDSNavMesh::GetGenerationTraceMask(void) const
{
	return MASK_PLAYERSOLID_BRUSHONLY;
}

CNavArea* CDoDSNavMesh::CreateArea(void) const
{
	return new CDoDSNavArea(GetNavPlace());
}
