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
	if (!CNavPrerequisite::IsEditing())
		return;

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
	if (!CNavPrerequisite::IsEditing())
		return;

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
	if (!CNavPrerequisite::IsEditing())
		return;

	auto& selected = TheNavMesh->GetSelectedPrerequisite();

	if (selected)
	{
		TheNavMesh->DeletePrerequisite(selected.get());
		TheNavMesh->NotifyDangerousEditCommandWasUsed();
	}
}

CON_COMMAND_F(sm_nav_prereq_draw_areas, "Draws nav areas affected by the currently selected prerequisite.", FCVAR_CHEAT)
{
	if (!CNavPrerequisite::IsEditing())
		return;

	auto& selected = TheNavMesh->GetSelectedPrerequisite();

	if (selected)
	{
		selected->DrawAreas();
	}
}

CON_COMMAND_F(sm_nav_prereq_set_origin, "Updates the selected nav prerequisite origin to your position.", FCVAR_CHEAT)
{
	if (!CNavPrerequisite::IsEditing())
		return;

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
	if (!CNavPrerequisite::IsEditing())
		return;

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
	if (!CNavPrerequisite::IsEditing())
		return;

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
	if (!CNavPrerequisite::IsEditing())
		return;

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
		Msg("ID: %i : %s \n", i, CNavPrerequisite::TaskIDtoString(static_cast<CNavPrerequisite::PrerequisiteTask>(i)));
	}
}

CON_COMMAND_F(sm_nav_prereq_set_goal_position, "Updates the selected nav prerequisite task goal position.", FCVAR_CHEAT)
{
	if (!CNavPrerequisite::IsEditing())
		return;

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
	if (!CNavPrerequisite::IsEditing())
		return;

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
	if (!CNavPrerequisite::IsEditing())
		return;

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
	if (!CNavPrerequisite::IsEditing())
		return;

	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_nav_prereq_set_toggle_condition <options ...> \n");
		META_CONPRINT(" Valid Options: \n  -clear -setvectorme -setvectordata -setintdata -setfloatdata -setentity -toggleinverted -settoggletypebyid \n");
		return;
	}

	auto& selected = TheNavMesh->GetSelectedPrerequisite();

	if (!selected)
	{
		META_CONPRINT("Select a nav prerequisite first! \n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	if (args.FindArg("-clear") != nullptr)
	{
		META_CONPRINTF("Toggle condition data for prerequisite %u cleared! \n", selected->GetID());
		selected->ClearToggleData();
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
		return;
	}

	if (args.FindArg("-setvectorme") != nullptr)
	{
		CBaseExtPlayer* host = extmanager->GetListenServerHost();
		Vector pos = host->GetAbsOrigin();
		selected->SetToggleData(pos);
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
		META_CONPRINTF("Prerequisite %u vector data set to <%s> \n", selected->GetID(), UtilHelpers::textformat::FormatVector(pos));
	}

	const char* szvecarg = args.FindArg("-setvectordata");

	if (szvecarg != nullptr)
	{
		Vector vec{ 0.0f, 0.0f, 0.0f };

		if (sscanf(szvecarg, "%f %f %f", &vec.x, &vec.y, &vec.z) == 3)
		{
			selected->SetToggleData(vec);
			META_CONPRINTF("Prerequisite %u vector data set to <%s> \n", selected->GetID(), UtilHelpers::textformat::FormatVector(vec));
		}
		else
		{
			META_CONPRINTF("Error: Could not parse a vector from \"%s\"! \n", szvecarg);
		}
	}

	int idata = args.FindArgInt("-setintdata", std::numeric_limits<int>::min());

	if (idata != std::numeric_limits<int>::min())
	{
		META_CONPRINTF("Prerequisite %u int data changed to \"%i\"! \n", selected->GetID(), idata);
		selected->SetToggleData(idata);
	}

	const char* szfloatarg = args.FindArg("-setfloatdata");

	if (szfloatarg != nullptr)
	{
		float val = atof(szfloatarg);
		META_CONPRINTF("Prerequisite %u float data changed to \"%g\"! \n", selected->GetID(), val);
		selected->SetToggleData(val);
	}

	int iEntIndex = args.FindArgInt("-setentity", INVALID_EHANDLE_INDEX);

	if (iEntIndex != INVALID_EHANDLE_INDEX)
	{
		CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(iEntIndex);

		if (!pEntity)
		{
			META_CONPRINTF("Error: NULL entity of index \"%i\"! \n", iEntIndex);
		}
		else
		{
			selected->SetToggleData(pEntity);
			META_CONPRINTF("Prerequisite %u toggle condition target entity set to %s!", selected->GetID(), UtilHelpers::textformat::FormatEntity(pEntity));
		}
	}

	if (args.FindArg("-toggleinverted") != nullptr)
	{
		selected->InvertToggleCondition();
		META_CONPRINTF("Prerequisite %u toggle condition is now %s! \n", selected->GetID(), selected->IsToggleConditionInverted() ? "INVERTED" : "NOT INVERTED");
	}

	int tctypeidx = args.FindArgInt("-settoggletypebyid", -1);

	if (tctypeidx != -1)
	{
		navscripting::ToggleCondition::TCTypes type;

		if (navscripting::ToggleCondition::IndexToTCType(tctypeidx, type))
		{
			selected->SetToggleData(type);
			META_CONPRINTF("Prerequisite %u toggle condition type changed to \"%s\"! \n", selected->GetID(), navscripting::ToggleCondition::TCTypeToString(type));
		}
	}

	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}