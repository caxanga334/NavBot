#include NAVBOT_PCH_FILE
#include <cstring>
#include <extension.h>
#include <util/commandargs_episode1.h>
#include <util/helpers.h>
#include <sdkports/debugoverlay_shared.h>
#include <entities/baseentity.h>
#include "nav_mesh.h"
#include "nav_volume.h"

ConVar sm_nav_volume_edit("sm_nav_volume_edit", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Set one to enable NavMesh Volume editing.");

CON_COMMAND_F(sm_nav_volume_create, "Creates a new Nav Volume at your position", FCVAR_CHEAT)
{
	if (!CNavVolume::IsEditing()) { return; }

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
	if (!CNavVolume::IsEditing()) { return; }

	DECLARE_COMMAND_ARGS;

	TheNavMesh->CommandNavMarkVolume(args);
}

CON_COMMAND_F(sm_nav_volume_deselect, "Deselects the nav volume.", FCVAR_CHEAT)
{
	TheNavMesh->SetSelectedVolume(nullptr);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_volume_delete, "Deletes the selected nav volume.", FCVAR_CHEAT)
{
	if (!CNavVolume::IsEditing()) { return; }

	TheNavMesh->RemoveSelectedVolume();
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
	TheNavMesh->NotifyDangerousEditCommandWasUsed();
}

CON_COMMAND_F(sm_nav_volume_draw_areas, "Draws areas inside the selected volume.", FCVAR_CHEAT)
{
	if (!CNavVolume::IsEditing()) { return; }

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
	if (!CNavVolume::IsEditing()) { return; }

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
	if (!CNavVolume::IsEditing()) { return; }

	DECLARE_COMMAND_ARGS;

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
	if (!CNavVolume::IsEditing()) { return; }

	DECLARE_COMMAND_ARGS;

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

CON_COMMAND_F(sm_nav_volume_set_toggle_condition, "Updates the selected nav prerequisite toggle condition", FCVAR_CHEAT)
{
	if (!CNavVolume::IsEditing()) { return; }

	DECLARE_COMMAND_ARGS;

	if (args.ArgC() < 2)
	{
		META_CONPRINT("[SM] Usage: sm_nav_volume_set_toggle_condition <options ...> \n");
		META_CONPRINT(" Valid Options: \n  -clear -setvectorme -setvectordata -setintdata -setfloatdata -setentity -toggleinverted -settoggletypebyid \n");
		return;
	}

	auto& selected = TheNavMesh->GetSelectedVolume();

	if (!selected)
	{
		META_CONPRINT("Select a nav volume first! \n");
		TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_ERROR);
		return;
	}

	if (args.FindArg("-clear") != nullptr)
	{
		META_CONPRINTF("Toggle condition data for volume %u cleared! \n", selected->GetID());
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
		META_CONPRINTF("Volume %u vector data set to <%s> \n", selected->GetID(), UtilHelpers::textformat::FormatVector(pos));
	}

	const char* szvecarg = args.FindArg("-setvectordata");

	if (szvecarg != nullptr)
	{
		Vector vec{ 0.0f, 0.0f, 0.0f };

		if (sscanf(szvecarg, "%f %f %f", &vec.x, &vec.y, &vec.z) == 3)
		{
			selected->SetToggleData(vec);
			META_CONPRINTF("Volume %u vector data set to <%s> \n", selected->GetID(), UtilHelpers::textformat::FormatVector(vec));
		}
		else
		{
			META_CONPRINTF("Error: Could not parse a vector from \"%s\"! \n", szvecarg);
		}
	}

	int idata = args.FindArgInt("-setintdata", std::numeric_limits<int>::min());

	if (idata != std::numeric_limits<int>::min())
	{
		META_CONPRINTF("Volume %u int data changed to \"%i\"! \n", selected->GetID(), idata);
		selected->SetToggleData(idata);
	}

	const char* szfloatarg = args.FindArg("-setfloatdata");

	if (szfloatarg != nullptr)
	{
		float val = atof(szfloatarg);
		META_CONPRINTF("Volume %u float data changed to \"%g\"! \n", selected->GetID(), val);
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
			META_CONPRINTF("Volume %u toggle condition target entity set to %s!", selected->GetID(), UtilHelpers::textformat::FormatEntity(pEntity));
		}
	}

	if (args.FindArg("-toggleinverted") != nullptr)
	{
		selected->InvertToggleCondition();
		META_CONPRINTF("Volume %u toggle condition is now %s! \n", selected->GetID(), selected->IsToggleConditionInverted() ? "INVERTED" : "NOT INVERTED");
	}

	int tctypeidx = args.FindArgInt("-settoggletypebyid", -1);

	if (tctypeidx != -1)
	{
		navscripting::ToggleCondition::TCTypes type;

		if (navscripting::ToggleCondition::IndexToTCType(tctypeidx, type))
		{
			selected->SetToggleData(type);
			META_CONPRINTF("Volume %u toggle condition type changed to \"%s\"! \n", selected->GetID(), navscripting::ToggleCondition::TCTypeToString(type));
		}
	}

	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F(sm_nav_volume_show_target_entity, "Shows the current target entity of the selected volume.", FCVAR_CHEAT)
{
	if (!CNavVolume::IsEditing()) { return; }

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
	if (!CNavVolume::IsEditing()) { return; }

	auto func1 = [](CNavVolume* volume1) {

		auto func2 = [&volume1](CNavVolume* volume2) {

			if (volume1->IntersectsWith(volume2))
			{
				Warning("Collision between nav volume #%i and nav volume #%i!\n", volume1->GetID(), volume2->GetID());
			}

			return true;
			};

		TheNavMesh->ForEveryNavVolume<CNavVolume>(func2);

		return true;
	};

	TheNavMesh->ForEveryNavVolume<CNavVolume>(func1);

	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}