#include NAVBOT_PCH_FILE
#include <mods/dods/dayofdefeatsourcemod.h>
#include <mods/dods/dodslib.h>
#include "dods_nav_mesh.h"
#include "dods_nav_waypoint.h"

static void DoDSWaypoint_Edit_AssignControlPoint(const CConCommandArgs& args)
{
	if (args.Count() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_dod_nav_waypoint_set_control_point <index> \n");
		return;
	}

	auto& wpt = TheNavMesh->GetSelectedWaypoint();

	if (!wpt)
	{
		META_CONPRINT("Error: No waypoint selected!\n");
		return;
	}

	int index = std::max(atoi(args[1]), dayofdefeatsource::INVALID_CONTROL_POINT);
	CDayOfDefeatSourceMod* mod = CDayOfDefeatSourceMod::GetDODMod();

	if (!mod->IsValidControlPointIndex(index))
	{
		META_CONPRINTF("Error: %s is not a valid control point index! \n", args[1]);
		return;
	}

	CDoDSWaypoint* dodwpt = static_cast<CDoDSWaypoint*>(wpt.get());
	dodwpt->AssignToControlPoint(index);
}

static void DoDSWaypoint_Edit_SetDodAttribs(const CConCommandArgs& args)
{
	if (args.Count() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_dod_nav_waypoint_toggle_dod_attribs <attrib 1> <attrib 2> ... <attrib M> \n");
		return;
	}

	auto& wpt = TheNavMesh->GetSelectedWaypoint();

	if (!wpt)
	{
		META_CONPRINT("Error: No waypoint selected!\n");
		return;
	}

	CDoDSWaypoint* dodwpt = static_cast<CDoDSWaypoint*>(wpt.get());

	for (int i = 1; i < args.Count(); i++)
	{
		const char* arg = args[i];
		CDoDSWaypoint::DoDWPAttributes attrib = CDoDSWaypoint::StringToDoDWPAttribute(arg);

		if (attrib == CDoDSWaypoint::DoDWPAttributes::DOD_WP_ATTRIBS_NONE)
		{
			META_CONPRINTF("%s is not a valid attribute name! \n", arg);
			continue;
		}

		if (dodwpt->HasDoDWaypointAttributes(attrib))
		{
			dodwpt->ClearDoDWaypointAttributes(attrib);
		}
		else
		{
			dodwpt->SetDoDWaypointAttributes(attrib);
		}
	}
}

static void DoDNav_WaypointAttribs_AutoComplete(const char* partialArg, int partialArgLength, SVCommandAutoCompletion& entries)
{
	for (auto& pair : CDoDSWaypoint::s_dodwpattribsmap)
	{
		if (V_strnicmp(pair.second.data(), partialArg, partialArgLength) == 0)
		{
			entries.AddAutoCompletionEntry(pair.second.data());
		}
	}
}

void CDoDSWaypoint::RegisterCommands()
{
	CServerCommandManager& manager = extmanager->GetServerCommandManager();

	manager.RegisterConCommand("sm_dod_nav_waypoint_set_control_point", "Assigns a control point to a waypoint.", FCVAR_GAMEDLL, DoDSWaypoint_Edit_AssignControlPoint);
	manager.RegisterConCommandAutoComplete("sm_dod_nav_waypoint_toggle_dod_attribs", "Toggles DoD specific waypoint attributes.", FCVAR_GAMEDLL,
		DoDSWaypoint_Edit_SetDodAttribs, DoDNav_WaypointAttribs_AutoComplete);
}