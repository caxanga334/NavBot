#include <extension.h>
#include <mods/blackmesa/blackmesadm_mod.h>
#include "bm_nav_mesh.h"

CBlackMesaNavMesh::CBlackMesaNavMesh() :
	CNavMesh()
{
	AddWalkableEntity("info_player_deathmatch");
	AddWalkableEntity("info_player_marine");
	AddWalkableEntity("info_player_scientist");

	navgenparams->half_human_width = 16.0f;
	navgenparams->half_human_height = 35.0f;
	navgenparams->human_height = 71.0f;
	navgenparams->human_crouch_height = 36.0f;
	navgenparams->human_eye_height = 72.0f;
	navgenparams->human_crouch_eye_height = 36.0f;
	navgenparams->jump_height = 49.0f;
	navgenparams->jump_crouch_height = 70.0f;
	navgenparams->climb_up_height = 70.0f;
	navgenparams->death_drop = 300.0f; // experimental
}

CBlackMesaNavMesh::~CBlackMesaNavMesh()
{
}

unsigned int CBlackMesaNavMesh::GetGenerationTraceMask(void) const
{
	return MASK_PLAYERSOLID_BRUSHONLY;
}
