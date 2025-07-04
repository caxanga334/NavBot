#include NAVBOT_PCH_FILE
#include <cstring>
#include <extension.h>
#include <extplayer.h>
#include <util/helpers.h>
#include <util/commandargs_episode1.h>
#include "nav_mesh.h"
#include "nav_area.h"
#include "nav_prereq.h"

CON_COMMAND_F(sm_nav_prereq_create, "Creates a new prerequisite.", FCVAR_CHEAT)
{
	edict_t* host = UtilHelpers::GetListenServerHost();
	const Vector& origin = host->GetCollideable()->GetCollisionOrigin();
	auto result = TheNavMesh->AddNavPrerequisite(&origin);

	if (result.has_value())
	{
		Msg("Created new prerequisite of ID #%i \n", result->get()->GetID());
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_SUCCESS);
	}
	else
	{
		Warning("Failed to create nav prerequisite! \n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
	}
}

CON_COMMAND_F(sm_nav_prereq_mark, "Selects/Deselects a nav prerequisite.", FCVAR_CHEAT)
{
	DECLARE_COMMAND_ARGS;

	auto& selected = TheNavMesh->GetSelectedPrerequisite();

	if (selected)
	{
		TheNavMesh->SetSelectedPrerequisite(nullptr);
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
	}
	else
	{
		if (args.ArgC() < 2)
		{
			// no id given, select nearest to host
			edict_t* host = UtilHelpers::GetListenServerHost();
			const Vector& origin = host->GetCollideable()->GetCollisionOrigin();
			TheNavMesh->SelectNearestPrerequisite(origin);
		}
		else
		{
			unsigned int id = static_cast<unsigned int>(atoi(args[1]));
			
			if (TheNavMesh->SelectPrerequisiteByID(id))
			{
				TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_SUCCESS);
			}
			else
			{
				Warning("Prerequisite of ID #%i not found! \n", id);
				TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
			}
		}
	}
}

CON_COMMAND_F(sm_nav_prereq_delete, "Deletes the currently selected prerequisite.", FCVAR_CHEAT)
{
	auto& selected = TheNavMesh->GetSelectedPrerequisite();

	if (selected)
	{
		TheNavMesh->DeletePrerequisite(selected.get());
		TheNavMesh->NotifyDangerousEditCommandWasUsed();
	}
}

CON_COMMAND_F(sm_nav_prereq_draw_areas, "Draws nav areas affected by the currently selected prerequisite.", FCVAR_CHEAT)
{
	auto& selected = TheNavMesh->GetSelectedPrerequisite();

	if (selected)
	{
		selected->DrawAreas();
	}
}

CON_COMMAND_F(sm_nav_prereq_set_origin, "Updates the selected nav prerequisite origin to your position.", FCVAR_CHEAT)
{
	edict_t* host = gamehelpers->EdictOfIndex(1);
	const Vector& origin = host->GetCollideable()->GetCollisionOrigin();

	auto& selected = TheNavMesh->GetSelectedPrerequisite();

	if (!selected)
	{
		Warning("No prerequisite selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	selected->SetOrigin(origin);
	selected->SearchForNavAreas();
	Msg("Updated prerequisite #%i origin to <%3.2f, %3.2f, %3.2f>\n", selected->GetID(), origin.x, origin.y, origin.z);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_prereq_set_bounds, "Updates the selected nav prerequisite bounds.", FCVAR_CHEAT)
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 6)
	{
		Msg("[SM] Usage: sm_nav_prereq_set_bounds <mins x> <mins y> <mins z> <maxs x> <maxs y> <maxs z>\n");
		return;
	}

	auto& selected = TheNavMesh->GetSelectedPrerequisite();

	if (!selected)
	{
		Warning("No prerequisite selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	Vector mins;
	Vector maxs;

	mins.x = atof(args[1]);
	mins.y = atof(args[2]);
	mins.z = atof(args[3]);
	maxs.x = atof(args[4]);
	maxs.y = atof(args[5]);
	maxs.z = atof(args[6]);

	selected->SetBounds(mins, maxs);
	selected->SearchForNavAreas();
	Msg("Updated prerequisite #%i bounds to mins [%3.2f, %3.2f, %3.2f] maxs [%3.2f, %3.2f, %3.2f]\n", selected->GetID(), mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_prereq_set_team, "Updates the selected nav prerequisite assigned team.", FCVAR_CHEAT)
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_prereq_set_team <team index>\n");
		Msg("    Use -2 for all teams. \n");
		return;
	}

	auto& selected = TheNavMesh->GetSelectedPrerequisite();

	if (!selected)
	{
		Warning("No prerequisite selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	int team = atoi(args[1]);
	selected->SetTeamIndex(team);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_prereq_set_task, "Updates the selected nav prerequisite goal task.", FCVAR_CHEAT)
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_prereq_set_task <task id>\n");
		return;
	}

	auto& selected = TheNavMesh->GetSelectedPrerequisite();

	if (!selected)
	{
		Warning("No prerequisite selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	int task = atoi(args[1]);
	selected->SetTask(task);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_prereq_list_available_tasks, "Lists all available task for nav prerequisites.", FCVAR_CHEAT)
{
	Msg("Nav Prerequisite tasks: \n");

	for (int i = static_cast<int>(CNavPrerequisite::PrerequisiteTask::TASK_NONE); i < static_cast<int>(CNavPrerequisite::PrerequisiteTask::MAX_TASK_TYPES); i++)
	{
		Msg("%i : %s \n", i, CNavPrerequisite::TaskIDtoString(static_cast<CNavPrerequisite::PrerequisiteTask>(i)));
	}
}

CON_COMMAND_F(sm_nav_prereq_set_goal_position, "Updates the selected nav prerequisite task goal position.", FCVAR_CHEAT)
{
	edict_t* host = gamehelpers->EdictOfIndex(1);
	const Vector& origin = host->GetCollideable()->GetCollisionOrigin();

	auto& selected = TheNavMesh->GetSelectedPrerequisite();

	if (!selected)
	{
		Warning("No prerequisite selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	selected->SetGoalPosition(origin);
	Msg("Updated prerequisite #%i goal position to <%3.2f, %3.2f, %3.2f>\n", selected->GetID(), origin.x, origin.y, origin.z);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_prereq_set_goal_entity, "Updates the selected nav prerequisite task goal entity.", FCVAR_CHEAT)
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_prereq_set_goal_entity <entity index>\n");
		return;
	}

	auto& selected = TheNavMesh->GetSelectedPrerequisite();

	if (!selected)
	{
		Warning("No prerequisite selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	int entindex = atoi(args[1]);

	if (entindex == 0) { return; }

	CBaseEntity* entity = gamehelpers->ReferenceToEntity(entindex);

	if (!entity)
	{
		Warning("No entity of index %i \n", entindex);
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	selected->AssignTaskEntity(entity);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_prereq_set_goal_data, "Updates the selected nav prerequisite task goal data.", FCVAR_CHEAT)
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_prereq_set_goal_data <value>\n");
		return;
	}

	auto& selected = TheNavMesh->GetSelectedPrerequisite();

	if (!selected)
	{
		Warning("No prerequisite selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	float data = atof(args[1]);
	selected->SetFloatData(data);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_prereq_set_toggle_condition, "Updates the selected nav prerequisite toggle condition", FCVAR_CHEAT)
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_prereq_set_toggle_condition clear\n   Clears all toggle condition data. \n");
		Msg("[SM] Usage: sm_nav_prereq_set_toggle_condition <int data> <float data> <ent index> <test condition> <inverted>\n   Set toggle condition values \n");
		Msg("[SM] Usage: sm_nav_prereq_set_toggle_condition setvectordata\n    Sets the toggle condition vector data to your current position. \n");
		Msg("For a list of condition types, use sm_nav_scripting_list_conditions. \n");
		return;
	}

	auto& selected = TheNavMesh->GetSelectedPrerequisite();

	if (!selected)
	{
		Warning("No prerequisite selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	const char* arg1 = args[1];

	if (std::strcmp(arg1, "clear") == 0)
	{
		selected->ClearToggleData();
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
		return;
	}
	else if (std::strcmp(arg1, "setvectordata") == 0)
	{
		edict_t* host = gamehelpers->EdictOfIndex(1);
		const Vector& origin = host->GetCollideable()->GetCollisionOrigin();
		selected->SetToggleData(origin);
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
		return;
	}

	if (args.ArgC() < 5)
	{
		Msg("[SM] Usage: sm_nav_prereq_set_toggle_condition <int data> <float data> <ent index> <test condition> <inverted>\n   Set toggle condition values \n");
		return;
	}

	int i = atoi(args[1]);
	float f = atof(args[2]);
	int ent = atoi(args[3]);
	int test = atoi(args[4]);
	bool inv = !!atoi(args[5]);

	selected->SetToggleData(i);
	selected->SetToggleData(f);

	CBaseEntity* pEntity;

	if (ent == 0)
	{
		pEntity = nullptr;
	}
	else
	{
		pEntity = gamehelpers->ReferenceToEntity(ent);
	}

	selected->SetToggleData(pEntity);

	navscripting::ToggleCondition::TCTypes type = navscripting::ToggleCondition::TCTypes::TYPE_NOT_SET;

	if (test > static_cast<int>(navscripting::ToggleCondition::TCTypes::TYPE_NOT_SET) && test < static_cast<int>(navscripting::ToggleCondition::TCTypes::MAX_TOGGLE_TYPES))
	{
		type = static_cast<navscripting::ToggleCondition::TCTypes>(test);
	}

	selected->SetToggleData(type);

	if (inv)
	{
		if (!selected->IsToggleConditionInverted())
		{
			selected->InvertToggleCondition();
		}
	}
	else
	{
		if (selected->IsToggleConditionInverted())
		{
			selected->InvertToggleCondition();
		}
	}

	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}