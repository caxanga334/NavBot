#include <extension.h>
#include <util/entprops.h>
#include "nav_mesh.h"
#include "nav_area.h"
#include "nav_elevator.h"

// This macros checks that there is a valid selected nav elevator and the marked nav area is registered on the elevator floor list
#define NAVELEV_VALIDMARKEDFLOOR															\
	auto& selected = TheNavMesh->GetSelectedElevator();										\
																							\
	if (selected.get() == nullptr)															\
	{																						\
		Msg("Select an elevator first!\n");													\
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);			\
		return;																				\
	}																						\
																							\
	CNavArea* area = TheNavMesh->GetMarkedArea();											\
																							\
	if (area == nullptr)																	\
	{																						\
		Warning("Use sm_nav_mark on the floor area first!\n");								\
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);			\
		return;																				\
	}																						\
																							\
	if (selected->GetFloorForArea(area) == nullptr)											\
	{																						\
		Warning("Marked area doesn't have a floor on the current elevator!\n");				\
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);			\
		return;																				\
	}																						\
																							\

ConVar sm_nav_elevator_edit("sm_nav_elevator_edit", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Set one to enable NavMesh Elevator editing.");

CON_COMMAND_F(sm_nav_elevator_create, "Creates a new Nav Elevator", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_elevator_create <elevator entity index>\n");
		return;
	}

	int index = atoi(args[1]);
	CBaseEntity* entity = gamehelpers->ReferenceToEntity(index);

	if (entity == nullptr || index == 0) // 0 is worldspawn and worldspawn is not an elevator
	{
		Warning("No entity of index #%i!\n", index);
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	auto elevator = TheNavMesh->AddNavElevator(entity);

	if (!elevator.has_value())
	{
		Warning("Failed to create a nav elevator!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	const CNavElevator* ne = elevator->get();

	Msg("Created Nav Elevator #%i.\n", ne->GetID());
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_elevator_select_nearest, "Selects the nearest elevator to your position.", FCVAR_CHEAT)
{
	edict_t* host = gamehelpers->EdictOfIndex(1);
	const Vector& origin = host->GetCollideable()->GetCollisionOrigin();

	TheNavMesh->SelectNearestElevator(origin);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_elevator_select_id, "Selects an elevator by id", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_elevator_select_id <id>\n");
		return;
	}

	int input = atoi(args[1]);

	if (input < 0)
	{
		Msg("ID numbers must be a positive number!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	unsigned int id = static_cast<unsigned int>(input);

	auto elevator = TheNavMesh->GetElevatorOfID<CNavElevator>(id);

	if (!elevator.has_value())
	{
		Warning("No elevator with ID %i found!\n", id);
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	TheNavMesh->SetSelectedElevator(elevator->get());
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_elevator_clear_selection, "Clears the selected elevator.", FCVAR_CHEAT)
{
	TheNavMesh->SetSelectedElevator(nullptr);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_elevator_delete, "Deletes the selected elevator.", FCVAR_CHEAT)
{
	auto& selected = TheNavMesh->GetSelectedElevator();

	if (selected.get() == nullptr)
	{
		Msg("Select an elevator first!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	TheNavMesh->DeleteElevator(selected.get());
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_elevator_toggle_type, "Toggles the elevator between automatic and manual types.", FCVAR_CHEAT)
{
	auto& selected = TheNavMesh->GetSelectedElevator();

	if (selected.get() == nullptr)
	{
		Msg("Select an elevator first!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	if (selected->GetType() == CNavElevator::ElevatorType::AUTO_TRIGGER)
	{
		selected->DetectType();
		selected->UpdateEntityProps();
		Msg("Current type was auto trigger, detecting type...\n");
	}
	else
	{
		selected->SetType(CNavElevator::ElevatorType::AUTO_TRIGGER);
		Msg("Setting elevator mode to auto trigger.\n");
	}

	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_elevator_update_position, "Updates the initial position of the elevator entity.", FCVAR_CHEAT)
{
	auto& selected = TheNavMesh->GetSelectedElevator();

	if (selected.get() == nullptr)
	{
		Msg("Select an elevator first!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	selected->UpdateElevatorInitialPosition();
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_elevator_add_floor, "Selects the nearest elevator to your position.", FCVAR_CHEAT)
{
	auto& selected = TheNavMesh->GetSelectedElevator();

	if (selected.get() == nullptr)
	{
		Msg("Select an elevator first!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	CNavArea* area = TheNavMesh->GetMarkedArea();

	if (area == nullptr)
	{
		Warning("To create a floor, you need to mark (sm_nav_mark) the area inside the elevator for this floor.\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	if (!selected->CreateNewFloor(area))
	{
		Warning("Create floor failed.\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	Msg("Created a new floor for Nav Area #%i on Elevator #%i\n", area->GetID(), selected->GetID());
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_elevator_set_floor_detection_distance, "Sets the minimum distance between the elevator and the floor position.", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_elevator_set_floor_detection_distance <distance>\n");
		return;
	}

	auto& selected = TheNavMesh->GetSelectedElevator();

	if (selected.get() == nullptr)
	{
		Msg("Select an elevator first!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	float dist = atof(args[1]);

	selected->SetFloorDetectionDistance(dist);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_elevator_set_team, "Sets the elevator team number.", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_elevator_set_team <int>\n");
		return;
	}

	auto& selected = TheNavMesh->GetSelectedElevator();

	if (selected.get() == nullptr)
	{
		Msg("Select an elevator first!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	int teamnum = atoi(args[1]);

	if (teamnum < 0 || teamnum >= MAX_TEAMS)
	{
		teamnum = NAV_TEAM_ANY;
	}

	selected->SetTeamNum(teamnum);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_elevator_delete_marked_floor, "Deletes the floor of the selected area.", FCVAR_CHEAT)
{
	NAVELEV_VALIDMARKEDFLOOR;

	selected->RemoveFloor(area);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_SUCCESS);
}

CON_COMMAND_F(sm_nav_elevator_delete_all_floors, "Deletes all elevator floors", FCVAR_CHEAT)
{
	auto& selected = TheNavMesh->GetSelectedElevator();

	if (selected.get() == nullptr)
	{
		Msg("Select an elevator first!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	selected->RemoveAllFloors();
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_SUCCESS);
}

CON_COMMAND_F(sm_nav_elevator_set_floor_toggle_state, "Sets the floor toggle state.", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_elevator_set_floor_toggle_state <toggle state>\n");
		return;
	}

	int ts = atoi(args[1]);

	if (ts != static_cast<int>(TS_AT_BOTTOM) && ts != static_cast<int>(TS_AT_TOP))
	{
		Warning("Invalid toggle state value %i\n    Valid values: 0 1\n", ts);
		return;
	}

	NAVELEV_VALIDMARKEDFLOOR;

	selected->SetFloorToggleState(area, ts);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_SUCCESS);
}

CON_COMMAND_F(sm_nav_elevator_set_floor_wait_position, "Sets the position bots will wait for the elevator.", FCVAR_CHEAT)
{
	NAVELEV_VALIDMARKEDFLOOR;

	edict_t* host = gamehelpers->EdictOfIndex(1);
	const Vector& origin = host->GetCollideable()->GetCollisionOrigin();

	selected->SetFloorWaitPosition(area, origin);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_SUCCESS);
}

CON_COMMAND_F(sm_nav_elevator_set_floor_position, "Sets the position used for detecting if the elevator is at this floor.", FCVAR_CHEAT)
{
	NAVELEV_VALIDMARKEDFLOOR;

	edict_t* host = gamehelpers->EdictOfIndex(1);
	const Vector& origin = host->GetCollideable()->GetCollisionOrigin();

	selected->SetFloorPosition(area, origin);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_SUCCESS);
}

CON_COMMAND_F(sm_nav_elevator_set_floor_call_button, "Sets the entity used for calling the elevator to this floor.", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_elevator_set_floor_call_button <entity index>\n");
		return;
	}

	NAVELEV_VALIDMARKEDFLOOR;

	int ref = atoi(args[1]);
	CBaseEntity* entity = gamehelpers->ReferenceToEntity(ref);
	
	if (entity == nullptr || ref == 0)
	{
		selected->SetFloorCallButton(area, nullptr);
		return;
	}

	selected->SetFloorCallButton(area, entity);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_SUCCESS);
}

CON_COMMAND_F(sm_nav_elevator_set_floor_use_button, "Sets the entity used for operating the elevator.", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_elevator_set_floor_use_button <entity index>\n");
		return;
	}

	NAVELEV_VALIDMARKEDFLOOR;

	int ref = atoi(args[1]);
	CBaseEntity* entity = gamehelpers->ReferenceToEntity(ref);

	if (entity == nullptr || ref == 0)
	{
		selected->SetFloorUseButton(area, nullptr);
		return;
	}

	selected->SetFloorUseButton(area, entity);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_SUCCESS);
}

CON_COMMAND_F(sm_nav_elevator_toggle_floor_shootable_button, "Toggle between use/shoot button type for the current marked floor.", FCVAR_CHEAT)
{
	NAVELEV_VALIDMARKEDFLOOR;

	auto floor = selected->GetFloorForArea(area);

	if (floor->shootable_button)
	{
		Msg("Setting button to useable.\n");
	}
	else
	{
		Msg("Setting button to shootable.\n");
	}

	selected->SetFloorShootableButton(area);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_SUCCESS);
}