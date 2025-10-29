#include NAVBOT_PCH_FILE
#include "zps_nav_mesh.h"

#if SOURCE_ENGINE == SE_SDK2013

static const std::unordered_map<std::string, CZPSNavArea::ZPSAttributes> s_zpsattribsmap = {
	{"nozombies", CZPSNavArea::ZPSAttributes::ZPS_ATTRIBUTE_NOZOMBIES},
	{"nohumans", CZPSNavArea::ZPSAttributes::ZPS_ATTRIBUTE_NOHUMANS},
	{"transient_humans", CZPSNavArea::ZPSAttributes::ZPS_ATTRIBUTE_TRANSIENT_SURVIVORSONLY},
	{"transient_physprops", CZPSNavArea::ZPSAttributes::ZPS_ATTRIBUTE_TRANSIENT_PHYSPROPS},
};

CZPSNavArea::ZPSAttributes CZPSNavArea::GetZPSAttributeFromName(const char* name)
{
	for (auto& pair : s_zpsattribsmap)
	{
		if (std::strcmp(name, pair.first.c_str()) == 0)
		{
			return pair.second;
		}
	}

	return CZPSNavArea::ZPSAttributes::ZPS_ATTRIBUTE_INVALID;
}

class CZPSNavEditToggleZPSAttribute
{
public:
	CZPSNavEditToggleZPSAttribute(CZPSNavArea::ZPSAttributes attribute)
	{
		m_attribute = attribute;
	}

	bool operator() (CZPSNavArea* area)
	{
		if (area->HasZPSAttributes(m_attribute))
		{
			area->ClearZPSAttributes(m_attribute);
		}
		else
		{
			area->SetZPSAttributes(m_attribute);
		}

		return true;
	}

private:
	CZPSNavArea::ZPSAttributes m_attribute;
};

static int ZPSAttributes_AutoComplete(const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
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

	for (auto& pair : s_zpsattribsmap)
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

CON_COMMAND_F_COMPLETION(sm_zps_nav_set_attribute, "Assigns ZPS specific nav area attributes", FCVAR_CHEAT | FCVAR_GAMEDLL, ZPSAttributes_AutoComplete)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: sm_zps_nav_set_attribute <attribute 1> ... <attribute N> \n");
		Msg("Valid Attributes: ");

		for (auto& i : s_zpsattribsmap)
		{
			Msg("%s ", i.first.c_str());
		}

		Msg("\n");

		return;
	}

	for (int i = 1; i < args.ArgC(); i++)
	{
		auto attrib = CZPSNavArea::GetZPSAttributeFromName(args[i]);

		if (attrib != CZPSNavArea::ZPSAttributes::ZPS_ATTRIBUTE_INVALID)
		{
			CZPSNavEditToggleZPSAttribute toggle{ attrib };
			TheNavMesh->ExecuteAreaEditCommand<CZPSNavArea, decltype(toggle)>(toggle, false);
			Msg("Toggling ZPS Attribute \"%s\". \n", args[i]);
		}
		else
		{
			Warning("Unknown ZPS Attribute \"%s\"! \n", args[i]);
		}
	}

	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
	TheNavMesh->ClearEditedAreas();
}

CON_COMMAND_F(sm_zps_nav_wipe_attributes, "Removes all ZPS attributes from the selected nav areas.", FCVAR_CHEAT | FCVAR_GAMEDLL)
{
	auto func = [](CZPSNavArea* area) { area->WipeZPSAttributes(); };

	TheNavMesh->ExecuteAreaEditCommand<CZPSNavArea, decltype(func)>(func);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

CON_COMMAND_F_COMPLETION(sm_zps_nav_select_areas_with_attributes, "Adds nav areas with the given ZPS attrib to the selected set.", FCVAR_CHEAT | FCVAR_GAMEDLL, ZPSAttributes_AutoComplete)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: sm_zps_nav_select_areas_with_attributes <attribute> \n");
		Msg("Valid Attributes: ");

		for (auto& i : s_zpsattribsmap)
		{
			Msg("%s ", i.first.c_str());
		}

		Msg("\n");

		return;
	}

	CZPSNavArea::ZPSAttributes attrib = CZPSNavArea::GetZPSAttributeFromName(args[1]);
	TheNavMesh->ClearSelectedSet();

	if (attrib != CZPSNavArea::ZPSAttributes::ZPS_ATTRIBUTE_INVALID)
	{
		auto func = [attrib](CNavArea* area) -> bool {

			if (static_cast<CZPSNavArea*>(area)->HasZPSAttributes(attrib))
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
		Warning("Unknown ZPS Attribute \"%s\"! \n", args[1]);
	}

	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
	TheNavMesh->SetMarkedArea(nullptr);
}

#endif // SOURCE_ENGINE == SE_SDK2013
