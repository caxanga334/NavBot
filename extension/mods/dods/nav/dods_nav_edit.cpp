#include NAVBOT_PCH_FILE
#include <string>
#include <mods/dods/nav/dods_nav_mesh.h>
#include <mods/dods/nav/dods_nav_area.h>

#ifdef DOD_DLL

static const std::unordered_map<std::string, CDODSNavArea::DoDNavAttributes> s_dodattribsmap = {
	{"noallies", CDODSNavArea::DODNAV_NO_ALLIES},
	{"noaxis", CDODSNavArea::DODNAV_NO_AXIS},
	{"blocked_until_bombed", CDODSNavArea::DODNAV_BLOCKED_UNTIL_BOMBED},
	{"blocked_without_bombs", CDODSNavArea::DODNAV_BLOCKED_WITHOUT_BOMBS},
	{"plant_bomb", CDODSNavArea::DODNAV_PLANT_BOMB},
	{"requires_prone", CDODSNavArea::DODNAV_REQUIRES_PRONE},
};

static CDODSNavArea::DoDNavAttributes NameToDoDAttributes(const std::string& name)
{
	auto it = s_dodattribsmap.find(name);

	if (it != s_dodattribsmap.end())
	{
		return it->second;
	}

	return CDODSNavArea::DoDNavAttributes::DODNAV_NONE;
}

class CDoDNavEditToggleDodAttribute
{
public:
	CDoDNavEditToggleDodAttribute(CDODSNavArea::DoDNavAttributes attribute)
	{
		m_attribute = attribute;
	}

	bool operator() (CDODSNavArea* area)
	{
		if (area->HasDoDAttributes(m_attribute))
		{
			area->ClearDoDAttributes(m_attribute);
		}
		else
		{
			area->SetDoDAttributes(m_attribute);
		}

		return true;
	}

private:
	CDODSNavArea::DoDNavAttributes m_attribute;
};

static int DoDAttributes_AutoComplete(const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	if (V_strlen(partial) >= COMMAND_COMPLETION_ITEM_LENGTH)
	{
		return 0;
	}

	char cmd[COMMAND_COMPLETION_ITEM_LENGTH + 2];
	V_strncpy(cmd, partial, sizeof(cmd));

	// skip to start of argument
	char* partialArg = V_strrchr(cmd, ' ');
	if (partialArg == NULL)
	{
		return 0;
	}

	// chop command from partial argument
	*partialArg = '\000';
	++partialArg;

	int partialArgLength = V_strlen(partialArg);

	int count = 0;

	for (auto& pair : s_dodattribsmap)
	{
		if (count >= COMMAND_COMPLETION_MAXITEMS)
		{
			break;
		}

		if (V_strnicmp(pair.first.c_str(), partialArg, partialArgLength) == 0)
		{
			V_snprintf(commands[count++], COMMAND_COMPLETION_ITEM_LENGTH, "%s %s", cmd, pair.first.c_str());
		}
	}

	return count;
}

CON_COMMAND_F_COMPLETION(sm_dod_nav_set_attribute, "Assigns DoD specific nav area attributes", FCVAR_CHEAT | FCVAR_GAMEDLL, DoDAttributes_AutoComplete)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: sm_dod_nav_set_attribute <attribute 1> ... <attribute N> \n");
		Msg("Valid Attributes: ");

		for (auto& i : s_dodattribsmap)
		{
			Msg("%s ", i.first.c_str());
		}

		Msg("\n");

		return;
	}

	for (int i = 1; i < args.ArgC(); i++)
	{
		auto attrib = NameToDoDAttributes(args[i]);

		if (attrib != CDODSNavArea::DoDNavAttributes::DODNAV_NONE)
		{
			CDoDNavEditToggleDodAttribute toggle{ attrib };
			TheNavMesh->ExecuteAreaEditCommand<CDODSNavArea, decltype(toggle)>(toggle, false);
			Msg("Toggling DoD Attribute \"%s\". \n", args[i]);
		}
		else
		{
			Warning("Unknown DoD Attribute \"%s\"! \n", args[i]);
		}
	}

	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
	TheNavMesh->ClearEditedAreas();
}

CON_COMMAND_F(sm_dod_nav_wipe_attributes, "Removes all DoD attributes from the selected nav areas.", FCVAR_CHEAT | FCVAR_GAMEDLL)
{
	auto func = [](CDODSNavArea* area) { area->WipeDoDAttributes(); };

	TheNavMesh->ExecuteAreaEditCommand<CDODSNavArea, decltype(func)>(func);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F_COMPLETION(sm_dod_nav_select_areas_with_attributes, "Adds nav areas with the given DoD attrib to the selected set.", FCVAR_CHEAT | FCVAR_GAMEDLL, DoDAttributes_AutoComplete)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: sm_dod_nav_select_areas_with_attributes <attribute> \n");
		Msg("Valid Attributes: ");

		for (auto& i : s_dodattribsmap)
		{
			Msg("%s ", i.first.c_str());
		}

		Msg("\n");

		return;
	}

	CDODSNavArea::DoDNavAttributes attrib = NameToDoDAttributes(args[1]);
	TheNavMesh->ClearSelectedSet();

	if (attrib != CDODSNavArea::DoDNavAttributes::DODNAV_NONE)
	{
		auto func = [attrib](CNavArea* area) -> bool {

			if (static_cast<CDODSNavArea*>(area)->HasDoDAttributes(attrib))
			{
				TheNavMesh->AddToSelectedSet(area);
			}

			return true;
		};
		
		CNavMesh::ForAllAreas(func);

		Msg("Marked areas with \"%s\" attribute.\n", args[1]);
	}
	else
	{
		Warning("Unknown DoD Attribute \"%s\"! \n", args[1]);
	}

	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
	TheNavMesh->SetMarkedArea(nullptr);
}

#endif // DOD_DLL
