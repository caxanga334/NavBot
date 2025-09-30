#include NAVBOT_PCH_FILE
#include <extension.h>
#include <mods/blackmesa/blackmesadm_mod.h>
#include "bm_nav_mesh.h"

CBlackMesaNavMesh::CBlackMesaNavMesh() :
	CNavMesh()
{
}

CBlackMesaNavMesh::~CBlackMesaNavMesh()
{
}

unsigned int CBlackMesaNavMesh::GetGenerationTraceMask(void) const
{
	return MASK_PLAYERSOLID_BRUSHONLY;
}
