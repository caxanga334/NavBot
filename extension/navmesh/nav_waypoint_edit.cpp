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
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_waypoint_mark_nearest, "Marks/select the waypoint nearest to you.", FCVAR_CHEAT)
{
	CBaseExtPlayer host(gamehelpers->EdictOfIndex(1));
	TheNavMesh->SelectNearestWaypoint(host.GetAbsOrigin());
}

CON_COMMAND_F(sm_nav_waypoint_mark_id, "Marks/select the waypoint of the given ID.", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_waypoint_mark_id <ID>\n");
		Msg("Selects the specific waypoint of ID.\n");
		return;
	}
	
	WaypointID id = static_cast<WaypointID>(atoi(args[1]));

	TheNavMesh->SelectWaypointofID(id);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_waypoint_unmark, "Clears the selected/marked waypoint.", FCVAR_CHEAT)
{
	TheNavMesh->ClearSelectedWaypoint();
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
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
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_CONNECT_FAIL);
		return;
	}

	auto fromWpt = TheNavMesh->GetWaypointOfID<CWaypoint>(firstID);
	auto toWpt = TheNavMesh->GetWaypointOfID<CWaypoint>(secondID);

	if (!fromWpt.has_value())
	{
		Warning("Waypoint with ID %i does not exists!\n", firstID);
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_CONNECT_FAIL);
		return;
	}

	if (!toWpt.has_value())
	{
		Warning("Waypoint with ID %i does not exists!\n", secondID);
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_CONNECT_FAIL);
		return;
	}

	fromWpt->get()->ConnectTo(toWpt->get());
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_waypoint_disconnect, "Removes a connection between two waypoints.", FCVAR_CHEAT)
{
	if (args.ArgC() < 3)
	{
		Msg("[SM] Usage: sm_nav_waypoint_disconnect <from ID> <to ID>\n");
		Msg("Removes a connection from the first waypoint to the second.\n");
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

	fromWpt->get()->DisconnectFrom(toWpt->get());
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_waypoint_info, "Shows information about the selected waypoint.", FCVAR_CHEAT)
{
	auto& waypoint = TheNavMesh->GetSelectedWaypoint();

	if (waypoint)
	{
		waypoint->PrintInfo();
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
	}
	else
	{
		Warning("ERROR: No waypoint selected!\n");
	}
}

CON_COMMAND_F(sm_nav_waypoint_set_radius, "Sets the waypoint radius.", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_waypoint_set_radius <radius>\n");
		return;
	}

	auto& waypoint = TheNavMesh->GetSelectedWaypoint();

	if (waypoint)
	{
		float radius = atof(args[1]);
		radius = std::clamp(radius, 0.0f, 512.0f);
		waypoint->SetRadius(radius);
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
	}
	else
	{
		Warning("No waypoint selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
	}
}

CON_COMMAND_F(sm_nav_waypoint_add_angle, "Adds a new angle to an waypoint.", FCVAR_CHEAT)
{
	CBaseExtPlayer host(gamehelpers->EdictOfIndex(1));
	auto& waypoint = TheNavMesh->GetSelectedWaypoint();

	if (waypoint)
	{
		if (waypoint->GetNumOfAvailableAngles() >= CWaypoint::MAX_AIM_ANGLES)
		{
			Warning("Waypoint at max angles capacity!\n");
			TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
			return;
		}

		QAngle eyeAngles = host.GetEyeAngles();
		waypoint->AddAngle(eyeAngles);
		Msg("Added angle <%3.2f, %3.2f, %3.2f> to waypoint #%i\n", eyeAngles.x, eyeAngles.y, eyeAngles.z, waypoint->GetID());
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
	}
	else
	{
		Warning("No waypoint selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
	}
}

CON_COMMAND_F(sm_nav_waypoint_update_angle, "Updates an existing angle on an waypoint.", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_waypoint_update_angle <angle index>\n");
		return;
	}

	int index = atoi(args[1]);

	if (index < 0 || index >= static_cast<int>(CWaypoint::MAX_AIM_ANGLES))
	{
		Warning("Index must be between 0 and %i\n", CWaypoint::MAX_AIM_ANGLES);
		return;
	}

	CBaseExtPlayer host(gamehelpers->EdictOfIndex(1));
	auto& waypoint = TheNavMesh->GetSelectedWaypoint();

	if (waypoint)
	{
		if (static_cast<std::size_t>(index) >= waypoint->GetNumOfAvailableAngles())
		{
			Warning("Index %i out of bounds. Use sm_nav_waypoint_add_angle!\n", index);
			TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
			return;
		}

		QAngle eyeAngles = host.GetEyeAngles();
		waypoint->SetAngle(eyeAngles, static_cast<std::size_t>(index));
		Msg("Updated angle [%i] <%3.2f, %3.2f, %3.2f> of waypoint #%i\n", index, eyeAngles.x, eyeAngles.y, eyeAngles.z, waypoint->GetID());
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
	}
	else
	{
		Warning("No waypoint selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
	}
}

CON_COMMAND_F(sm_nav_waypoint_clear_angles, "Removes all angles from an waypoint.", FCVAR_CHEAT)
{
	auto& waypoint = TheNavMesh->GetSelectedWaypoint();

	if (waypoint)
	{
		waypoint->ClearAngles();
		Msg("Removed all angles from waypoint #%i\n", waypoint->GetID());
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
	}
	else
	{
		Warning("No waypoint selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
	}
}

CON_COMMAND_F(sm_nav_waypoint_set_team, "Sets the waypoint owning team.", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_waypoint_set_team <team index>\n");
		return;
	}

	int teamnum = atoi(args[1]);

	if (teamnum < TEAM_UNASSIGNED)
	{
		teamnum = NAV_TEAM_ANY;
	}

	auto& waypoint = TheNavMesh->GetSelectedWaypoint();

	if (waypoint)
	{
		waypoint->SetTeam(teamnum);
		Msg("Waypoint #%i team set to %i.\n", waypoint->GetID(), teamnum);
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
	}
	else
	{
		Warning("No waypoint selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
	}
}

CON_COMMAND_F(sm_nav_waypoint_warp_to, "Teleports you to the selected waypoint.", FCVAR_CHEAT)
{
	auto& waypoint = TheNavMesh->GetSelectedWaypoint();

	if (waypoint)
	{
		CBaseExtPlayer host(gamehelpers->EdictOfIndex(1));
		host.Teleport(waypoint->GetOrigin());
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
	}
	else
	{
		Warning("No waypoint selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
	}
}

CON_COMMAND_F(sm_nav_waypoint_view_angle, "Sets your current angle to the given angle index", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_waypoint_view_angle <angle index>\n");
		return;
	}

	int index = atoi(args[1]);

	if (index < 0 || index >= static_cast<int>(CWaypoint::MAX_AIM_ANGLES))
	{
		Warning("Index must be between 0 and %i\n", CWaypoint::MAX_AIM_ANGLES);
		return;
	}

	auto& waypoint = TheNavMesh->GetSelectedWaypoint();

	if (waypoint)
	{
		if (static_cast<size_t>(index) >= waypoint->GetNumOfAvailableAngles())
		{
			Warning("Invalid angle index %i\n", index);
			TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
			return;
		}

		CBaseExtPlayer host(gamehelpers->EdictOfIndex(1));
		host.SnapEyeAngles(waypoint->GetAngle(static_cast<size_t>(index)));
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
	}
	else
	{
		Warning("No waypoint selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
	}
}

