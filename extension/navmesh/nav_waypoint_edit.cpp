#include <extension.h>
#include <extplayer.h>
#include <sdkports/sdk_traces.h>
#include "nav_mesh.h"
#include "nav_waypoint.h"

// Valve's mathlib defs that conflicts with STD
#undef min
#undef max
#undef clamp

ConVar sm_nav_waypoint_edit("sm_nav_waypoint_edit", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Set one to enable NavMesh Waypoint editing.");

CON_COMMAND_F(sm_nav_waypoint_add, "Adds a new waypoint at your position.", FCVAR_CHEAT)
{
	edict_t* host = gamehelpers->EdictOfIndex(1);
	const Vector& origin = host->GetCollideable()->GetCollisionOrigin();
	
	auto wpt = TheNavMesh->AddWaypoint(origin);

	if (wpt.has_value())
	{
		Msg("Add waypoint %i\n", wpt->get()->GetID());
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_WAYPOINT_ADD);
	}
	else
	{
		Warning("Add waypoint failed!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
	}
}

CON_COMMAND_F(sm_nav_waypoint_delete, "Deletes the nearest or selected waypoint.", FCVAR_CHEAT)
{
	edict_t* host = gamehelpers->EdictOfIndex(1);
	const Vector& origin = host->GetCollideable()->GetCollisionOrigin();

	auto& selectedWpt = TheNavMesh->GetSelectedWaypoint();

	if (selectedWpt)
	{
		Msg("Selected waypoint %i deleted.\n", selectedWpt->GetID());
		TheNavMesh->RemoveWaypoint(selectedWpt.get());
		TheNavMesh->ClearSelectedWaypoint();
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_SUCCESS);
		return;
	}

	CWaypoint* nearest = nullptr;
	float best = std::numeric_limits<float>::max();

	TheNavMesh->ForEveryWaypoint<CWaypoint>([&origin, &best, &nearest](CWaypoint* waypoint) {
		float distance = (waypoint->GetOrigin() - origin).Length();

		if (distance <= CWaypoint::WAYPOINT_DELETE_SEARCH_DIST && distance < best)
		{
			nearest = waypoint;
			best = distance;
		}

		return true;
	});

	if (nearest != nullptr)
	{
		Msg("Deleting nearest waypoint %i\n", nearest->GetID());
		TheNavMesh->RemoveWaypoint(nearest);
		TheNavMesh->ClearSelectedWaypoint();
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_SUCCESS);
	}
	else
	{
		Warning("No waypoint within %3.2f units could be found!\n", CWaypoint::WAYPOINT_DELETE_SEARCH_DIST);
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
	}
}

CON_COMMAND_F(sm_nav_waypoint_mark_crosshair, "Marks/select the waypoint nearest to your crosshair.", FCVAR_CHEAT)
{
	CBaseExtPlayer host(gamehelpers->EdictOfIndex(1));

	Vector forward;
	
	host.EyeVectors(&forward);
	forward.NormalizeInPlace();

	trace_t tr;
	trace::line(host.GetEyeOrigin(), host.GetEyeOrigin() + (forward * CWaypoint::WAYPOINT_EDIT_DRAW_RANGE), MASK_SHOT, tr);
	
	TheNavMesh->SelectNearestWaypoint(tr.endpos);
}

CON_COMMAND_F(sm_nav_waypoint_mark_nearest, "Marks/select the waypoint nearest to you.", FCVAR_CHEAT)
{
	CBaseExtPlayer host(gamehelpers->EdictOfIndex(1));
	TheNavMesh->SelectNearestWaypoint(host.GetAbsOrigin());
}

CON_COMMAND_F(sm_nav_waypoint_unmark, "Clears the selected/marked waypoint.", FCVAR_CHEAT)
{
	TheNavMesh->ClearSelectedWaypoint();
}

CON_COMMAND_F(sm_nav_waypoint_connect, "Adds a connection between two waypoints.", FCVAR_CHEAT)
{
	if (args.ArgC() < 3)
	{
		Msg("[SM] Usage: sm_nav_waypoint_connect <from ID> <to ID>\n");
		Msg("Adds a connection from the first waypoint to the second.\n");
		return;
	}

	WaypointID firstID = static_cast<WaypointID>(atoi(args[1]));
	WaypointID secondID = static_cast<WaypointID>(atoi(args[2]));

	if (firstID == secondID)
	{
		Warning("Connection must be between two different waypoints\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	auto fromWpt = TheNavMesh->GetWaypointOfID<CWaypoint>(firstID);
	auto toWpt = TheNavMesh->GetWaypointOfID<CWaypoint>(secondID);

	if (!fromWpt.has_value())
	{
		Warning("Waypoint with ID %i does not exists!\n", firstID);
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	if (!toWpt.has_value())
	{
		Warning("Waypoint with ID %i does not exists!\n", secondID);
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	fromWpt->get()->ConnectTo(toWpt->get());
}