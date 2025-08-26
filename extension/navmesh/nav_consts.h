#ifndef NAVBOT_NAV_MESH_CONSTS_H_
#define NAVBOT_NAV_MESH_CONSTS_H_
#pragma once

#include <cstdint>

constexpr int NAV_TEAM_ANY = -2;
constexpr unsigned int NAV_TEAMS_ARRAY_SIZE = 8U; // array size for per team data, max teams is 32, until we see a mod use more than 5 teams, we will use 8.

enum class OffMeshConnectionType : std::uint32_t
{
	OFFMESH_INVALID = 0,
	OFFMESH_GROUND, // solid ground
	OFFMESH_TELEPORTER, // map based teleporter (trigger_teleport)
	OFFMESH_BLAST_JUMP, // Blast/Rocket Jump
	OFFMESH_DOUBLE_JUMP, // Double jump (climb)
	OFFMESH_JUMP_OVER_GAP, // Jump over gap
	OFFMESH_CLIMB_UP, // Climb/Jump
	OFFMESH_DROP_DOWN, // Drop from ledge
	OFFMESH_GRAPPLING_HOOK, // Grappling hook
	OFFMESH_CATAPULT, // Catapult/push
	OFFMESH_STRAFE_JUMP, // A jump that requires strafing over an obstacle

	MAX_OFFMESH_CONNECTION_TYPES // max known link types
};

#endif // !NAVBOT_NAV_MESH_CONSTS_H_

