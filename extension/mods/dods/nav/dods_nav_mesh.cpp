#include NAVBOT_PCH_FILE
#include <extension.h>
#include "dods_nav_area.h"
#include "dods_nav_mesh.h"

CDODSNavMesh::CDODSNavMesh() :
	CNavMesh()
{
	AddWalkableEntity("dod_bomb_dispenser", true);
	AddWalkableEntity("dod_bomb_target", false);
	AddWalkableEntity("dod_capture_area", true);
	AddWalkableEntity("dod_location", false);
	AddWalkableEntity("dod_control_point", false);
	AddWalkableEntity("info_player_allies", false);
	AddWalkableEntity("info_player_axis", false);
	AddWalkableEntity("func_team_wall", true);
	AddWalkableEntity("func_teamblocker", true);

	navgenparams->half_human_width = 16.0f;
	navgenparams->half_human_height = 35.0f;
	navgenparams->human_height = 71.0f;
	navgenparams->human_crouch_height = 45.0f;
	navgenparams->human_eye_height = 72.0f;
	navgenparams->human_crouch_eye_height = 34.0f;
	navgenparams->jump_height = 49.0f;
	navgenparams->jump_crouch_height = 70.0f;
	navgenparams->climb_up_height = 200.0f;
	navgenparams->death_drop = 250.0f;
	
	// no need to listen for round start events, mod notify us
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
