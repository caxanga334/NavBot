#include <extension.h>
#include <sdkports/sdk_traces.h>
#include "tfnavarea.h"
#include "tfnavmesh.h"

#if SOURCE_ENGINE == SE_TF2

static const std::unordered_map<std::string, CTFNavArea::TFNavPathAttributes> s_tfnavpathattribs = {
	{"noredteam", CTFNavArea::TFNAV_PATH_NO_RED_TEAM},
	{"nobluteam", CTFNavArea::TFNAV_PATH_NO_BLU_TEAM},
	{"noflagcarriers", CTFNavArea::TFNAV_PATH_NO_CARRIERS},
	{"flagcarriersavoid", CTFNavArea::TFNAV_PATH_CARRIERS_AVOID},
	{"spawnroom", CTFNavArea::TFNAV_PATH_DYNAMIC_SPAWNROOM},
};

inline static CTFNavArea::TFNavPathAttributes NameToTFNavPathAttribute(std::string name)
{
	auto it = s_tfnavpathattribs.find(name);

	if (it != s_tfnavpathattribs.end())
	{
		return it->second;
	}

	return CTFNavArea::TFNAV_PATH_INVALID;
}

static const std::unordered_map<std::string, CTFNavArea::TFNavAttributes> s_tfnavattribs = {
	{"limit_red", CTFNavArea::TFNAV_LIMIT_TO_REDTEAM},
	{"limit_blu", CTFNavArea::TFNAV_LIMIT_TO_BLUTEAM},
};

inline static CTFNavArea::TFNavAttributes NameToTFNavAttributes(std::string name)
{
	auto it = s_tfnavattribs.find(name);

	if (it != s_tfnavattribs.end())
	{
		return it->second;
	}

	return CTFNavArea::TFNAV_INVALID;
}

static const std::unordered_map<std::string, CTFNavArea::MvMNavAttributes> s_mvmattribs = {
	{"frontlines", CTFNavArea::MVMNAV_FRONTLINES},
	{"upgradestation", CTFNavArea::MVMNAV_UPGRADESTATION},
};

inline static CTFNavArea::MvMNavAttributes NameToMvMAttributes(std::string name)
{
	auto it = s_mvmattribs.find(name);

	if (it != s_mvmattribs.end())
	{
		return it->second;
	}

	return CTFNavArea::MVMNAV_INVALID;
}

class CTFNavEditTogglePathAttribute
{
public:
	CTFNavEditTogglePathAttribute(CTFNavArea::TFNavPathAttributes attribute)
	{
		m_attribute = attribute;
	}

	inline bool operator() (CNavArea* baseArea)
	{
		CTFNavArea* area = static_cast<CTFNavArea*>(baseArea);

		if (TheNavMesh->IsSelectedSetEmpty() && area->HasTFPathAttributes(m_attribute))
		{
			area->ClearTFPathAttributes(m_attribute);
		}
		else
		{
			area->SetTFPathAttributes(m_attribute);
		}

		return true;
	}

private:
	CTFNavArea::TFNavPathAttributes m_attribute;
};

class CTFNavEditToggleAttribute
{
public:
	CTFNavEditToggleAttribute(CTFNavArea::TFNavAttributes attribute)
	{
		m_attribute = attribute;
	}

	bool operator() (CNavArea* baseArea)
	{
		CTFNavArea* area = static_cast<CTFNavArea*>(baseArea);

		if (TheNavMesh->IsSelectedSetEmpty() && area->HasTFAttributes(m_attribute))
		{
			area->ClearTFAttributes(m_attribute);
		}
		else
		{
			area->SetTFAttributes(m_attribute);
		}

		return true;
	}

private:
	CTFNavArea::TFNavAttributes m_attribute;
};

class CTFNavEditToggleMvMAttribute
{
public:
	CTFNavEditToggleMvMAttribute(CTFNavArea::MvMNavAttributes attribute)
	{
		m_attribute = attribute;
	}

	bool operator() (CNavArea* baseArea)
	{
		CTFNavArea* area = static_cast<CTFNavArea*>(baseArea);

		if (TheNavMesh->IsSelectedSetEmpty() && area->HasMVMAttributes(m_attribute))
		{
			area->ClearMVMAttributes(m_attribute);
		}
		else
		{
			area->SetMVMAttributes(m_attribute);
		}

		return true;
	}

private:
	CTFNavArea::MvMNavAttributes m_attribute;
};

static int PathAttributes_AutoComplete(const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
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

	for (auto& pair : s_tfnavpathattribs)
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

CON_COMMAND_F_COMPLETION(sm_tf_nav_toggle_path_attrib, "Toggles NavBot TF Path Attributes on the selected set", FCVAR_CHEAT, PathAttributes_AutoComplete)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: sm_tf_nav_toggle_path_attrib <attribute 1> ... <attribute N> \n");
		Msg("Valid Attributes: ");

		for (auto& i : s_tfnavpathattribs)
		{
			Msg("%s ", i.first.c_str());
		}

		Msg("\n");

		return;
	}

	for (int i = 1; i < args.ArgC(); i++)
	{
		auto attrib = NameToTFNavPathAttribute(args[i]);

		if (attrib != CTFNavArea::TFNAV_PATH_INVALID)
		{
			CTFNavEditTogglePathAttribute toggle(attrib);
			TheNavMesh->ForAllSelectedAreas(toggle);
			Msg("Toggling TF Path Attribute \"%s\". \n", args[i]);
		}
		else
		{
			Warning("Unknown TF Path Attribute \"%s\"! \n", args[i]);
		}
	}

	TheNavMesh->ClearSelectedSet();
}

CON_COMMAND_F(sm_tf_nav_toggle_attrib, "Toggles NavBot TF Attributes on the selected set", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: sm_tf_nav_toggle_attrib <attribute 1> ... <attribute N> \n");
		Msg("Valid Attributes: ");

		for (auto& i : s_tfnavattribs)
		{
			Msg("%s ", i.first.c_str());
		}

		Msg("\n");

		return;
	}

	for (int i = 1; i < args.ArgC(); i++)
	{
		auto attrib = NameToTFNavAttributes(args[i]);

		if (attrib != CTFNavArea::TFNAV_INVALID)
		{
			CTFNavEditToggleAttribute toggle(attrib);
			TheNavMesh->ForAllSelectedAreas(toggle);
			Msg("Toggling TF Attribute \"%s\". \n", args[i]);
		}
		else
		{
			Warning("Unknown TF Attribute \"%s\"! \n", args[i]);
		}
	}

	TheNavMesh->ClearSelectedSet();
}

static int MvMAttributes_AutoComplete(const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
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

	for (auto& pair : s_mvmattribs)
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

CON_COMMAND_F_COMPLETION(sm_tf_nav_toggle_mvm_attrib, "Toggles NavBot TF MvM Attributes on the selected set", FCVAR_CHEAT, MvMAttributes_AutoComplete)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: sm_tf_nav_toggle_mvm_attrib <attribute 1> ... <attribute N> \n");
		Msg("Valid Attributes: ");

		for (auto& i : s_mvmattribs)
		{
			Msg("%s ", i.first.c_str());
		}

		Msg("\n");

		return;
	}

	for (int i = 1; i < args.ArgC(); i++)
	{
		auto attrib = NameToMvMAttributes(args[i]);

		if (attrib != CTFNavArea::MVMNAV_INVALID)
		{
			CTFNavEditToggleMvMAttribute toggle(attrib);
			TheNavMesh->ForAllSelectedAreas(toggle);
			Msg("Toggling TF MvM Attribute \"%s\". \n", args[i]);
		}
		else
		{
			Warning("Unknown TF MvM Attribute \"%s\"! \n", args[i]);
		}
	}

	TheNavMesh->ClearSelectedSet();
}

CON_COMMAND_F(sm_tf_nav_auto_set_spawnrooms, "Detects and set spawn room areas automatically.", FCVAR_CHEAT)
{
	int areas = 0;

	auto functor = [&areas](CNavArea* baseArea) {
		CTFNavArea* area = static_cast<CTFNavArea*>(baseArea);
		Vector center = area->GetCenter();
		center.z += 24.0f;
		bool inside = false;

		auto functor2 = [&inside, &center](int index, edict_t* edict, CBaseEntity* entity) {
			if (edict != nullptr && edict->GetCollideable() != nullptr)
			{
				if (trace::pointwithin(edict->GetCollideable(), center))
				{
					inside = true;
					return false; // exit search
				}
			}

			return true; // keep searching for entities
		};

		UtilHelpers::ForEachEntityOfClassname("func_respawnroom", functor2);

		if (inside)
		{
			areas++;
			area->SetTFPathAttributes(CTFNavArea::TFNAV_PATH_DYNAMIC_SPAWNROOM);
		}
		
		return true;
	};

	TheNavMesh->ForAllAreas(functor);

	Msg("Added spawn room attribute to %i nav areas!\n", areas);
}

#endif