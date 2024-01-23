#include <stdexcept>
#include <filesystem>

#include <extension.h>
#include <util/helpers.h>
#include <util/librandom.h>
#include <mods/tf2/tf2lib.h>
#include "teamfortress2_shareddefs.h"
#include "tf2_class_selection.h"

CTF2ClassSelection::CTF2ClassSelection()
{
}

CTF2ClassSelection::~CTF2ClassSelection()
{
}

void CTF2ClassSelection::LoadClassSelectionData()
{
	char filepath[PLATFORM_MAX_PATH];
	smutils->BuildPath(SourceMod::Path_SM, filepath, sizeof(filepath), "configs/navbot/tf/class_selection.cfg");

	if (!std::filesystem::exists(filepath))
	{
		smutils->LogError(myself, "Failed to load Class Selection config file! File \"%s\" does not exists!", filepath);
		return;
	}

	m_parserdata.BeginParse();
	SourceMod::SMCStates state{};
	auto errorcode = textparsers->ParseFile_SMC(filepath, this, &state);

	if (errorcode != SourceMod::SMCError_Okay)
	{
		smutils->LogError(myself, "Failed to parse Class Selection config file! File \"%s\"! Error: %s", filepath, textparsers->GetSMCErrorString(errorcode));
		return;
	}

	smutils->LogMessage(myself, "Loaded Class Selection config file.");
}

TeamFortress2::TFClassType CTF2ClassSelection::SelectAClass(TeamFortress2::TFTeam team, ClassRosterType roster) const
{
	std::vector<TeamFortress2::TFClassType> available_classes;
	available_classes.reserve(9);
	bool has_priority = false; // signals priority

	for (int i = 0; i < static_cast<int>(TeamFortress2::TFClassType::TFClass_Engineer); i++)
	{
		TeamFortress2::TFClassType current_class = static_cast<TeamFortress2::TFClassType>(i + 1); // array starts a 0, classes starts at 1

		int onteam = UtilHelpers::GetNumberofPlayersOnTeam(static_cast<int>(team));
		int asclass = tf2lib::GetNumberOfPlayersAsClass(current_class, team);
		const auto& data = m_classdata[static_cast<int>(roster)][i];

		if (data.IsDisabled())
			continue; // class is disabled for selection

		if (data.HasMaximum() && asclass >= data.GetMaximum())
			continue; // at or over class limit

		if (onteam < data.GetTeamSize())
			continue; // not enough players on team

		if (data.HasMinimum() && asclass < data.GetMinimum())
		{
			// number of players as class below minimum, priorize!
			has_priority = true;
			available_classes.push_back(current_class);
			continue;
		}

		if (data.HasRate() && asclass < data.GetMinimumForRate(onteam))
		{
			// number of players as class below minimum based on per player class rate, priorize!
			has_priority = true;
			available_classes.push_back(current_class);
			continue;
		}

		// the current class is available but is not on a priority condition
		if (has_priority)
			continue;

		available_classes.push_back(current_class);
	}

	if (available_classes.size() == 0)
	{
		return TeamFortress2::TFClassType::TFClass_Unknown;
	}

	if (available_classes.size() == 1)
	{
		return available_classes[0];
	}

	return available_classes[librandom::generate_random_uint(0, available_classes.size() - 1)];
}

/**
 * @brief Checks if the given class is above the class limit for bots
 * @param tfclass Class to check
 * @param team Team to check
 * @param roster Class roster to use
 * @return true if above limit, false otherwise
*/
bool CTF2ClassSelection::IsClassAboveLimit(TeamFortress2::TFClassType tfclass, TeamFortress2::TFTeam team, ClassRosterType roster) const
{
	int index = static_cast<int>(tfclass - 1); // array starts a 0, classes starts at 1
	const auto& data = m_classdata[static_cast<int>(roster)][index];

	if (data.HasMaximum())
	{
		int asclass = tf2lib::GetNumberOfPlayersAsClass(tfclass, team);

		if (asclass > data.GetMaximum())
		{
			return true;
		}
	}

	return false;
}

SMCResult CTF2ClassSelection::ReadSMC_NewSection(const SMCStates* states, const char* name)
{
	switch (m_parserdata.depth)
	{
	case 0: // parsen hasn't entered any section yet
	{
		if (strncasecmp(name, "TFClassSelection", 16) == 0)
		{
			m_parserdata.depth = 1; // Initial Section, valid file
			return SourceMod::SMCResult_Continue;
		}
		else
		{
			smutils->LogError(myself, "Error parsing class_selection.cfg, expected \"TFClassSelection\" but got \"%s\"!", name);
			return SourceMod::SMCResult_Halt;
		}
	}
	case 1: // entered 'TFClassSelection' section
	{
		if (strncasecmp(name, "default", 7) == 0)
		{
			m_parserdata.depth = 2; // entering roster section
			m_parserdata.current_roster = ClassRosterType::ROSTER_DEFAULT;
			return SourceMod::SMCResult_Continue;
		}
		else if (strncasecmp(name, "attackers", 9) == 0)
		{
			m_parserdata.depth = 2;
			m_parserdata.current_roster = ClassRosterType::ROSTER_ATTACKERS;
			return SourceMod::SMCResult_Continue;
		}
		else if (strncasecmp(name, "defenders", 9) == 0)
		{
			m_parserdata.depth = 2;
			m_parserdata.current_roster = ClassRosterType::ROSTER_DEFENDERS;
			return SourceMod::SMCResult_Continue;
		}
		else if (strncasecmp(name, "mannvsmachine", 13) == 0)
		{
			m_parserdata.depth = 2;
			m_parserdata.current_roster = ClassRosterType::ROSTER_MANNVSMACHINE;
			return SourceMod::SMCResult_Continue;
		}
		else
		{
			smutils->LogError(myself, "Error parsing class_selection.cfg, unknown section name \"%s\"!", name);
			return SourceMod::SMCResult_Halt;
		}
	}
	case 2: // entered roster section
	{
		std::string classname(name);
		auto classtype = tf2lib::GetClassTypeFromName(classname);

		if (classtype == TeamFortress2::TFClassType::TFClass_Unknown)
		{
			smutils->LogError(myself, "Unknown class type \"%s\"!", name);
			return SourceMod::SMCResult_Halt;
		}

		m_parserdata.depth = 3; // class section
		m_parserdata.current_class = classtype;
		m_parserdata.current_data.Init(); // clear data
		return SourceMod::SMCResult_Continue;
	}
	default:
	{
		break;
	}
	}

	return SourceMod::SMCResult_Continue;
}

SMCResult CTF2ClassSelection::ReadSMC_KeyValue(const SMCStates* states, const char* key, const char* value)
{
	if (m_parserdata.depth != 3) // Depth 3 is class data section, we are only excepting key values at depth 3
	{
		smutils->LogError(myself, "Unexpected key value pair \"<%s : %s>\" !", key, value);
		return SourceMod::SMCResult_Halt;
	}

	if (strncasecmp(key, "minimum", 7) == 0)
	{
		m_parserdata.current_data.minimum = atoi(value);
	}
	else if (strncasecmp(key, "maximum", 7) == 0)
	{
		m_parserdata.current_data.maximum = atoi(value);
	}
	else if (strncasecmp(key, "rate", 4) == 0)
	{
		m_parserdata.current_data.rate = atoi(value);
	}
	else if (strncasecmp(key, "teamsize", 8) == 0)
	{
		m_parserdata.current_data.teamsize = atoi(value);
	}
	else
	{
		smutils->LogError(myself, "Unknown key \"%s\"!", key);
		return SourceMod::SMCResult_Halt;
	}

	return SourceMod::SMCResult_Continue;
}

SMCResult CTF2ClassSelection::ReadSMC_LeavingSection(const SMCStates* states)
{
	switch (m_parserdata.depth)
	{
	case 3: // finished reading class data section
	{
		StoreClassData(m_parserdata.current_roster, m_parserdata.current_class, m_parserdata.current_data);
		m_parserdata.depth -= 1;
		break;
	}
	default:
		m_parserdata.depth -= 1;
		break;
	}

	return SourceMod::SMCResult_Continue;
}

void CTF2ClassSelection::StoreClassData(ClassRosterType roster, TeamFortress2::TFClassType classtype, const ClassData& data)
{
	int r = static_cast<int>(roster);
	int c = static_cast<int>(classtype);
	c -= 1; // remember, valid tfclasstypes goes from 1 to 9 but the array size is 9, subtract 1

	if (r < 0 || r >= static_cast<int>(ClassRosterType::MAX_ROSTER_TYPES))
	{
		throw std::out_of_range("Out of Bounds write attempt at roster type!");
	}

	if (c < 0 || c >= static_cast<int>(TeamFortress2::TFClassType::TFClass_Engineer))
	{
		throw std::out_of_range("Out of Bounds write attempt at class type!");
	}

	m_classdata[r][classtype] = data;
}

CTF2ClassSelection::ClassData::ClassData()
{
	minimum = 0;
	maximum = 0;
	rate = 0;
	teamsize = 0;
}
