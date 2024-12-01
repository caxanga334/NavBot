#include "nav.h"

// Default values goes here.
// From: https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/game/server/nav.h
// DO NOT IFDEF HERE!
// Override these values at the mod's specific NavMesh constructor

CNavMeshGeneratorParameters::CNavMeshGeneratorParameters()
{
	generation_step_size = 25.0f;
	jump_height = 41.8f;
	jump_crouch_height = 64.0f;
	step_height = 18.0f;
	death_drop = 400.0f;
	climb_up_height = 64.0f;
	half_human_width = 16.0f;
	half_human_height = 35.5f;
	human_height = 71.0f;
	human_eye_height = 62.0f;
	human_crouch_height = 55.0f;
	human_crouch_eye_height = 37.0f;
}

static CNavMeshGeneratorParameters s_navgenparams;
CNavMeshGeneratorParameters* navgenparams = &s_navgenparams;