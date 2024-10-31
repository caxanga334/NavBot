#include <extension.h>
#include "tfnavmesh.h"
#include "tfnav_waypoint.h"

// Don't registers these commands on non TF2 mods
#if SOURCE_ENGINE == SE_TF2

CON_COMMAND_F(sm_nav_tf_waypoint_set_control_point, "Associate a control point index to the selected waypoint.", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_tf_waypoint_set_control_point <cp index>\n");
		Msg("Waypoint must be selected first.\nUse \"sm_navbot_tf_list_control_points\" to get a list of control points.\n");
		Msg("Pass -1 to remove association.\n");
		return;
	}

	int cpindex = atoi(args[1]);

	if (cpindex < CTFWaypoint::NO_CONTROL_POINT || cpindex >= MAX_CONTROL_POINTS)
	{
		Warning("Invalid control point index \"%i\"!", cpindex);
		cpindex = CTFWaypoint::NO_CONTROL_POINT;
	}

	auto& wptptr = TheNavMesh->GetSelectedWaypoint();

	if (wptptr.get() == nullptr)
	{
		Warning("Error: No waypoint selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	CTFWaypoint* wpt = static_cast<CTFWaypoint*>(wptptr.get());
	wpt->SetControlPointIndex(cpindex);
}

#endif // SOURCE_ENGINE == SE_TF2
