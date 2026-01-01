#include NAVBOT_PCH_FILE
#include "zps_nav_mesh.h"

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

static void ZPSEdit_Attributes_AutoComplete(const char* partialArg, int partialArgLength, SVCommandAutoCompletion& entries)
{
	for (auto& pair : s_zpsattribsmap)
	{
		if (V_strnicmp(pair.first.c_str(), partialArg, partialArgLength) == 0)
		{
			entries.AddAutoCompletionEntry(pair.first.c_str());
		}
	}
}

static void sm_zps_nav_set_attribute(const CConCommandArgs& args)
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

static void sm_zps_nav_wipe_attributes(const CConCommandArgs&)
{
	auto func = [](CZPSNavArea* area) { area->WipeZPSAttributes(); };

	TheNavMesh->ExecuteAreaEditCommand<CZPSNavArea, decltype(func)>(func);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

static void sm_zps_nav_select_areas_with_attributes(const CConCommandArgs& args)
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

void CZPSNavMesh::RegisterModCommands()
{
	CServerCommandManager& manager = extmanager->GetServerCommandManager();

	manager.RegisterConCommand("sm_zps_nav_wipe_attributes", "Removes all ZPS attributes from the selected nav areas.", FCVAR_CHEAT | FCVAR_GAMEDLL, sm_zps_nav_wipe_attributes);
	manager.RegisterConCommandAutoComplete("sm_zps_nav_set_attribute", "Assigns ZPS specific nav area attributes.", FCVAR_CHEAT | FCVAR_GAMEDLL, sm_zps_nav_set_attribute, ZPSEdit_Attributes_AutoComplete);
	manager.RegisterConCommandAutoComplete("sm_zps_nav_select_areas_with_attributes", "Adds nav areas with the given ZPS attrib to the selected set.", FCVAR_CHEAT | FCVAR_GAMEDLL, sm_zps_nav_select_areas_with_attributes, ZPSEdit_Attributes_AutoComplete);
}
