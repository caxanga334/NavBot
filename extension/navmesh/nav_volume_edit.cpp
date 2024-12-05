#include <extension.h>
#include <util/helpers.h>
#include <sdkports/debugoverlay_shared.h>
#include <entities/baseentity.h>
#include "nav_mesh.h"
#include "nav_volume.h"

ConVar sm_nav_volume_edit("sm_nav_volume_edit", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Set one to enable NavMesh Volume editing.");

CON_COMMAND_F(sm_nav_volume_create, "Creates a new Nav Volume at your position", FCVAR_CHEAT)
{
	edict_t* host = gamehelpers->EdictOfIndex(1);
	const Vector& origin = host->GetCollideable()->GetCollisionOrigin();

	auto newVolume = TheNavMesh->AddNavVolume(origin);

	if (newVolume.has_value())
	{
		Msg("Created new nav volume with ID %i\n", newVolume->get()->GetID());
		newVolume->get()->SearchForNavAreas();
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_SUCCESS);
	}
	else
	{
		Warning("Failed to create a new volume!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
	}
}

CON_COMMAND_F(sm_nav_volume_select, "Selects a nav volume.", FCVAR_CHEAT)
{
	TheNavMesh->CommandNavMarkVolume(args);
}

CON_COMMAND_F(sm_nav_volume_deselect, "Deselects the nav volume.", FCVAR_CHEAT)
{
	TheNavMesh->SetSelectedVolume(nullptr);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_volume_delete, "Deletes the selected nav volume.", FCVAR_CHEAT)
{
	TheNavMesh->RemoveSelectedVolume();
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_volume_draw_areas, "Draws areas inside the selected volume.", FCVAR_CHEAT)
{
	auto& selectedVolume = TheNavMesh->GetSelectedVolume();

	if (!selectedVolume)
	{
		Warning("No volume selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	selectedVolume->DrawAreas();
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_volume_set_origin, "Updates the selected nav volume origin to your position.", FCVAR_CHEAT)
{
	edict_t* host = gamehelpers->EdictOfIndex(1);
	const Vector& origin = host->GetCollideable()->GetCollisionOrigin();

	auto& selectedVolume = TheNavMesh->GetSelectedVolume();

	if (!selectedVolume)
	{
		Warning("No volume selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	selectedVolume->SetOrigin(origin);
	selectedVolume->SearchForNavAreas();
	Msg("Updated volume #%i origin to <%3.2f, %3.2f, %3.2f>\n", selectedVolume->GetID(), origin.x, origin.y, origin.z);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_volume_set_bounds, "Updates the selected nav volume bounds.", FCVAR_CHEAT)
{
	if (args.ArgC() < 6)
	{
		Msg("[SM] Usage: sm_nav_volume_set_bounds <mins x> <mins y> <mins z> <maxs x> <maxs y> <maxs z>\n");
		return;
	}

	auto& selectedVolume = TheNavMesh->GetSelectedVolume();

	if (!selectedVolume)
	{
		Warning("No volume selected!\n");
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

	selectedVolume->SetBounds(mins, maxs);
	selectedVolume->SearchForNavAreas();
	Msg("Updated volume #%i bounds to mins [%3.2f, %3.2f, %3.2f] maxs [%3.2f, %3.2f, %3.2f]\n", selectedVolume->GetID(), mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_volume_set_team, "Updates the selected nav volume assigned team.", FCVAR_CHEAT)
{
	int team = NAV_TEAM_ANY;

	if (args.ArgC() < 2)
	{
		Msg("No team index specified, setting to NO TEAM.\n");
	}
	else
	{
		team = atoi(args[1]);

		if (team < 0 || team >= static_cast<int>(NAV_TEAMS_ARRAY_SIZE))
		{
			team = NAV_TEAM_ANY;
		}

		Msg("Team index = %i\n", team);
	}

	auto& selectedVolume = TheNavMesh->GetSelectedVolume();

	if (!selectedVolume)
	{
		Warning("No volume selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	selectedVolume->SetTeam(team);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_volume_set_check_condition_type, "Updates the selected nav volume condition type.", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_volume_set_check_condition_type <type ID>\n");
		Msg("Get a list of all types using:\n    sm_nav_volume_list_condition_types\n");
		return;
	}

	auto& selectedVolume = TheNavMesh->GetSelectedVolume();

	if (!selectedVolume)
	{
		Warning("No volume selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	auto type = CNavVolume::IntToConditionType(atoi(args[1]));

	selectedVolume->SetConditionType(type);
	Msg("Condition type for Nav Volume #%i changed to %s\n", selectedVolume->GetID(), CNavVolume::GetNameOfConditionType(type));
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_volume_set_check_condition_inverted, "Updates the selected nav volume to use inverted condition check logic.", FCVAR_CHEAT)
{
	auto& selectedVolume = TheNavMesh->GetSelectedVolume();

	if (!selectedVolume)
	{
		Warning("No volume selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	bool inverted = selectedVolume->IsConditionCheckInverted();

	selectedVolume->SetInvertedConditionCheck(!inverted);
	Msg("Inverted condition logic for Nav Volume #%i\n", selectedVolume->GetID());
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_volume_set_check_target_entity, "Updates the selected nav volume to use inverted condition check logic.", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_volume_set_check_target_entity <entity targetname or index>\n");
		return;
	}

	auto& selectedVolume = TheNavMesh->GetSelectedVolume();

	if (!selectedVolume)
	{
		Warning("No volume selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	int entindex = atoi(args[1]);

	if (entindex != 0) // do not allow world
	{
		CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(entindex);

		if (pEntity == nullptr)
		{
			Warning("Entity of index #%i could not be found!\n", entindex);
			TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
			return;
		}

		selectedVolume->SetTargetEntity(pEntity);
	}
	else // atoi returned 0, assume it's a targetname
	{
		entindex = UtilHelpers::FindEntityByTargetname(INVALID_EHANDLE_INDEX, args[1]);
		CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(entindex);

		if (pEntity == nullptr)
		{
			Warning("Entity with targetname \"%s\" could not be found!\n", args[1]);
			TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
			return;
		}

		selectedVolume->SetTargetEntity(pEntity);
	}

	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_volume_set_check_float_data, "Updates the selected nav volume entity check float data", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("[SM] Usage: sm_nav_volume_set_check_float_data <value>\n");
		return;
	}

	auto& selectedVolume = TheNavMesh->GetSelectedVolume();

	if (!selectedVolume)
	{
		Warning("No volume selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	float value = atof(args[1]);
	selectedVolume->SetEntFloatData(value);
	Msg("Entity float data for Nav Volume #%i updated to %3.2f\n", selectedVolume->GetID(), value);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_volume_show_target_entity, "Shows the current target entity of the selected volume.", FCVAR_CHEAT)
{
	auto& selectedVolume = TheNavMesh->GetSelectedVolume();

	if (!selectedVolume)
	{
		Warning("No volume selected!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	CBaseEntity* pEntity = selectedVolume->GetTargetEntity();

	if (pEntity == nullptr)
	{
		Warning("Target entity is NULL!\n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	entities::HBaseEntity be(pEntity);
	Vector center = be.WorldSpaceCenter();
	NDebugOverlay::EntityBounds(pEntity, 0, 128, 0, 200, 10.0f);
	NDebugOverlay::Line(selectedVolume->GetOrigin(), center, 255, 0, 255, true, 10.0f);
	float distance = (selectedVolume->GetOrigin() - center).Length();
	Msg("Distance from Volume origin to entity center: %3.4f\n", distance);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_volume_check_for_conflicts, "Checks if there are volume conflicts", FCVAR_CHEAT)
{
	TheNavMesh->ForEveryNavVolume<CNavVolume>([](CNavVolume* volume1) {
		TheNavMesh->ForEveryNavVolume<CNavVolume>([&volume1](CNavVolume* volume2) {

			if (volume1->IntersectsWith(volume2))
			{
				Warning("Collision between nav volume #%i and nav volume #%i!\n", volume1->GetID(), volume2->GetID());
			}

			return true;
		});

		return true;
	});

	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND(sm_nav_volume_list_condition_types, "Shows a list of condition types IDs.")
{
	Msg("All available nav volume condition types: \n");

	for (int i = 0; i < static_cast<int>(CNavVolume::ConditionType::MAX_CONDITION_TYPES); i++)
	{
		const char* name = CNavVolume::GetNameOfConditionType(static_cast<CNavVolume::ConditionType>(i));
		Msg("  #%i : %s\n", i, name);
	}
}