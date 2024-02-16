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