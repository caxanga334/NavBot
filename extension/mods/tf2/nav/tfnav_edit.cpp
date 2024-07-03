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
	{"sentry_gun_hint", CTFNavArea::TFNAV_SENTRYGUN_HINT},
	{"dispenser_hint", CTFNavArea::TFNAV_DISPENSER_HINT},
	{"tele_entrance_hint", CTFNavArea::TFNAV_TELE_ENTRANCE_HINT},
	{"tele_exit_hint", CTFNavArea::TFNAV_TELE_EXIT_HINT},
	{"sniper_spot_hint", CTFNavArea::TFNAV_SNIPER_HINT},
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

CON_COMMAND_F(sm_tf_nav_toggle_path_attrib, "Toggles NavBot TF Path Attributes on the selected set", FCVAR_CHEAT)
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

CON_COMMAND_F(sm_tf_nav_toggle_mvm_attrib, "Toggles NavBot TF MvM Attributes on the selected set", FCVAR_CHEAT)
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

		UtilHelpers::ForEachEntityOfClassname("func_respawnroom", [&inside, &center](int index, edict_t* edict, CBaseEntity* entity) {
			if (edict != nullptr && edict->GetCollideable() != nullptr)
			{
				if (trace::pointwithin(edict->GetCollideable(), center))
				{
					inside = true;
					return false; // exit search
				}
			}

			return true; // keep searching for entities
		});

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