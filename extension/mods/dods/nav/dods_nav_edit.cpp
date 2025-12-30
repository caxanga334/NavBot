#include NAVBOT_PCH_FILE
#include <string>
#include <mods/dods/nav/dods_nav_mesh.h>
#include <mods/dods/nav/dods_nav_area.h>

static const std::unordered_map<std::string, CDoDSNavArea::DoDNavAttributes> s_dodattribsmap = {
	{"noallies", CDoDSNavArea::DODNAV_NO_ALLIES},
	{"noaxis", CDoDSNavArea::DODNAV_NO_AXIS},
	{"deprecated1", CDoDSNavArea::DODNAV_DEPRECATED1},
	{"deprecated2", CDoDSNavArea::DODNAV_DEPRECATED2},
	{"bombs_to_open", CDoDSNavArea::DODNAV_BOMBS_TO_OPEN},
	{"requires_prone", CDoDSNavArea::DODNAV_REQUIRES_PRONE},
};

static CDoDSNavArea::DoDNavAttributes NameToDoDAttributes(const std::string& name)
{
	auto it = s_dodattribsmap.find(name);

	if (it != s_dodattribsmap.end())
	{
		return it->second;
	}

	return CDoDSNavArea::DoDNavAttributes::DODNAV_NONE;
}

class CDoDNavEditToggleDodAttribute
{
public:
	CDoDNavEditToggleDodAttribute(CDoDSNavArea::DoDNavAttributes attribute)
	{
		m_attribute = attribute;
	}

	bool operator() (CDoDSNavArea* area)
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
	CDoDSNavArea::DoDNavAttributes m_attribute;
};

static void DoDNavEdit_DodAttributes_AutoComplete(const char* partialArg, int partialArgLength, SVCommandAutoCompletion& entries)
{
	for (auto& pair : s_dodattribsmap)
	{
		if (V_strnicmp(pair.first.c_str(), partialArg, partialArgLength) == 0)
		{
			entries.AddAutoCompletionEntry(pair.first.c_str());
		}
	}
}

static void sm_dod_nav_set_attribute(const SVCommandArgs& args)
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

		if (attrib != CDoDSNavArea::DoDNavAttributes::DODNAV_NONE)
		{
			CDoDNavEditToggleDodAttribute toggle{ attrib };
			TheNavMesh->ExecuteAreaEditCommand<CDoDSNavArea, decltype(toggle)>(toggle, false);
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

static void sm_dod_nav_wipe_attributes(const SVCommandArgs& args)
{
	auto func = [](CDoDSNavArea* area) { area->WipeDoDAttributes(); };

	TheNavMesh->ExecuteAreaEditCommand<CDoDSNavArea, decltype(func)>(func);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

static void sm_dod_nav_select_areas_with_attributes(const SVCommandArgs& args)
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

	CDoDSNavArea::DoDNavAttributes attrib = NameToDoDAttributes(args[1]);
	TheNavMesh->ClearSelectedSet();

	if (attrib != CDoDSNavArea::DoDNavAttributes::DODNAV_NONE)
	{
		auto func = [attrib](CNavArea* area) -> bool {

			if (static_cast<CDoDSNavArea*>(area)->HasDoDAttributes(attrib))
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

void CDoDSNavMesh::RegisterEditCommands()
{
	CServerCommandManager& manager = extmanager->GetServerCommandManager();

	manager.RegisterConCommand("sm_dod_nav_wipe_attributes", "Removes all DoD attributes from the selected nav areas.", FCVAR_CHEAT | FCVAR_GAMEDLL, sm_dod_nav_wipe_attributes);
	manager.RegisterConCommandAutoComplete("sm_dod_nav_set_attribute", "Assigns DoD specific nav area attributes.", FCVAR_CHEAT | FCVAR_GAMEDLL, sm_dod_nav_set_attribute, DoDNavEdit_DodAttributes_AutoComplete);
	manager.RegisterConCommandAutoComplete("sm_dod_nav_select_areas_with_attributes", "Adds nav areas with the given DoD attrib to the selected set.", FCVAR_CHEAT | FCVAR_GAMEDLL, sm_dod_nav_select_areas_with_attributes, DoDNavEdit_DodAttributes_AutoComplete);
}
