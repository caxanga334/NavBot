#include NAVBOT_PCH_FILE
#include "../css_shareddefs.h"
#include "css_nav_mesh.h"

static std::unordered_map<std::string, CCSSNavArea::CSSAttributes> s_cssattribsmap = {
	{"no_hostages", CCSSNavArea::CSSAttributes::CSSATTRIB_NO_HOSTAGES},
	{"no_ts", CCSSNavArea::CSSAttributes::CSSATTRIB_NO_T},
	{"no_cts", CCSSNavArea::CSSAttributes::CSSATTRIB_NO_CT},
};

CCSSNavArea::CSSAttributes CCSSNavArea::NameToCSSAttrib(const char* name)
{
	auto it = s_cssattribsmap.find(name);

	if (it != s_cssattribsmap.end())
	{
		return it->second;
	}

	return CCSSNavArea::CSSAttributes::CSSATTRIB_NONE;
}

static void CSSNavEdit_CSAttributes_AutoComplete(const char* partialArg, int partialArgLength, SVCommandAutoCompletion& entries)
{
	for (auto& pair : s_cssattribsmap)
	{
		if (V_strnicmp(pair.first.c_str(), partialArg, partialArgLength) == 0)
		{
			entries.AddAutoCompletionEntry(pair.first.c_str());
		}
	}
}

static void sm_css_nav_toggle_attribute(const CConCommandArgs& args)
{
	if (args.ArgC() < 2)
	{
		META_CONPRINT("Usage: sm_css_nav_toggle_attribute <attribute 1> ... <attribute N> \n");
		META_CONPRINT("Valid Attributes: \n  ");

		for (auto& i : s_cssattribsmap)
		{
			META_CONPRINTF("%s ", i.first.c_str());
		}

		META_CONPRINT("\n");

		return;
	}

	for (int i = 1; i < args.ArgC(); i++)
	{
		CCSSNavArea::CSSAttributes attrib = CCSSNavArea::NameToCSSAttrib(args[i]);

		if (attrib != CCSSNavArea::CSSAttributes::CSSATTRIB_NONE)
		{
			auto func = [attrib](CCSSNavArea* area) { area->ToggleCSAttributes(attrib); };
			TheNavMesh->ExecuteAreaEditCommand<CCSSNavArea>(func, false);
			META_CONPRINTF("Toggling CSS Attribute \"%s\". \n", args[i]);
		}
		else
		{
			META_CONPRINTF("Unknown CSS Attribute \"%s\"! \n", args[i]);
		}
	}

	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
	TheNavMesh->ClearEditedAreas();
}

static void sm_css_nav_wipe_attribute(const CConCommandArgs& args)
{
	auto func = [](CCSSNavArea* area) { area->WipeAllCSAttributes(); };
	TheNavMesh->ExecuteAreaEditCommand<CCSSNavArea>(func, true);
	TheNavMesh->PlayEditSound(CNavMesh::EditSoundType::SOUND_GENERIC_BLIP);
}

void CCSSNavMesh::RegisterCSSNavAreaEditCommands() const
{
	CServerCommandManager& manager = extmanager->GetServerCommandManager();

	manager.RegisterConCommandAutoComplete("sm_css_nav_toggle_attribute", "Toggles CSS attributes on nav areas.", FCVAR_GAMEDLL | FCVAR_CHEAT,
		sm_css_nav_toggle_attribute, CSSNavEdit_CSAttributes_AutoComplete);
	manager.RegisterConCommand("sm_css_nav_wipe_attribute", "Wipes all CSS attributes from nav areas.", FCVAR_GAMEDLL | FCVAR_CHEAT, sm_css_nav_wipe_attribute);
}