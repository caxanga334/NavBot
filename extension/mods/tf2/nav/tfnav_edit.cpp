#include <extension.h>
#include "tfnavarea.h"
#include "tfnavmesh.h"

inline static CTFNavArea::TFNavPathAttributes NameToTFNavPathAttribute(const char* name)
{
	if (strncasecmp(name, "noredteam", 9) == 0)
	{
		return CTFNavArea::TFNAV_PATH_NO_RED_TEAM;
	}
	else if (strncasecmp(name, "nobluteam", 9) == 0)
	{
		return CTFNavArea::TFNAV_PATH_NO_BLU_TEAM;
	}
	else if (strncasecmp(name, "noflagcarriers", 14) == 0)
	{
		return CTFNavArea::TFNAV_PATH_NO_CARRIERS;
	}
	else if (strncasecmp(name, "flagcarriersavoid", 17) == 0)
	{
		return CTFNavArea::TFNAV_PATH_CARRIERS_AVOID;
	}
	else if (strncasecmp(name, "spawnroom", 9) == 0)
	{
		return CTFNavArea::TFNAV_PATH_DYNAMIC_SPAWNROOM;
	}
	else
	{
		return CTFNavArea::TFNAV_PATH_INVALID;
	}
}

inline static CTFNavArea::TFNavAttributes NameToTFNavAttributes(const char* name)
{
	if (strncasecmp(name, "limit_red", 9) == 0)
	{
		return CTFNavArea::TFNAV_LIMIT_TO_REDTEAM;
	}
	else if (strncasecmp(name, "limit_blu", 9) == 0)
	{
		return CTFNavArea::TFNAV_LIMIT_TO_BLUTEAM;
	}
	else if (strncasecmp(name, "sentry_gun_hint", 15) == 0)
	{
		return CTFNavArea::TFNAV_SENTRYGUN_HINT;
	}
	else if (strncasecmp(name, "dispenser_hint", 14) == 0)
	{
		return CTFNavArea::TFNAV_DISPENSER_HINT;
	}
	else if (strncasecmp(name, "tele_entrance_hint", 18) == 0)
	{
		return CTFNavArea::TFNAV_TELE_ENTRANCE_HINT;
	}
	else if (strncasecmp(name, "tele_exit_hint", 14) == 0)
	{
		return CTFNavArea::TFNAV_TELE_EXIT_HINT;
	}
	else if (strncasecmp(name, "sniper_spot_hint", 16) == 0)
	{
		return CTFNavArea::TFNAV_SNIPER_HINT;
	}
	else
	{
		return CTFNavArea::TFNAV_INVALID;
	}
}

inline static CTFNavArea::MvMNavAttributes NameToMvMAttributes(const char* name)
{
	if (strncasecmp(name, "frontlines", 10) == 0)
	{
		return CTFNavArea::MVMNAV_FRONTLINES;
	}
	else
	{
		return CTFNavArea::MVMNAV_INVALID;
	}
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
		Msg("Valid Attributes: noredteam nobluteam noflagcarriers flagcarriersavoid spawnroom \n");
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
		Msg("Valid Attributes: limit_red limit_blu sentry_gun_hint dispenser_hint tele_entrance_hint tele_exit_hint sniper_spot_hint \n");
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
		Msg("Valid Attributes: frontlines \n");
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