#include NAVBOT_PCH_FILE
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
	smutils->BuildPath(SourceMod::Path_SM, filepath, sizeof(filepath), "configs/navbot/tf/class_selection.custom.cfg");

	if (!std::filesystem::exists(filepath))
	{
		smutils->BuildPath(SourceMod::Path_SM, filepath, sizeof(filepath), "configs/navbot/tf/class_selection.cfg");

		if (!std::filesystem::exists(filepath))
		{
			smutils->LogError(myself, "Failed to load Class Selection config file! File \"%s\" does not exists!", filepath);
			return;
		}
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
	std::vector<TeamFortress2::TFClassType> priority_classes;
	available_classes.reserve(9);
	bool has_priority = false; // signals priority

	tf2lib::TeamClassData teamdata{};
	teamdata.team = team;
	UtilHelpers::ForEachPlayer<tf2lib::TeamClassData>(teamdata); // fill data

	for (int i = 0; i < static_cast<int>(TeamFortress2::TFClassType::TFClass_Engineer); i++)
	{
		TeamFortress2::TFClassType current_class = static_cast<TeamFortress2::TFClassType>(i + 1); // array starts a 0, classes starts at 1

		int asclass = teamdata.GetNumAsClass(current_class);
		const auto& data = m_classdata[static_cast<int>(roster)][i];

		if (data.IsDisabled())
			continue; // class is disabled for selection

		if (data.HasMaximum() && asclass >= data.GetMaximum())
			continue; // at or over class limit

		if (teamdata.players_on_team < data.GetTeamSize())
			continue; // not enough players on team

		if (data.HasMinimum() && asclass < data.GetMinimum())
		{
			// number of players as class below minimum, priorize!
			has_priority = true;
			priority_classes.push_back(current_class);
			continue;
		}

		if (data.HasRate() && asclass < data.GetMinimumForRate(teamdata.players_on_team))
		{
			// number of players as class below minimum based on per player class rate, priorize!
			has_priority = true;
			priority_classes.push_back(current_class);
			continue;
		}

		// the current class is available but is not on a priority condition
		if (has_priority)
			continue;

		available_classes.push_back(current_class);
	}

	if (!priority_classes.empty())
	{
		available_classes.swap(priority_classes);
	}

	if (available_classes.empty())
	{
		return TeamFortress2::TFClassType::TFClass_Unknown;
	}

#ifdef EXT_DEBUG
	rootconsole->ConsolePrint("Available Classes");
	rootconsole->ConsolePrint("Team %i Roster %i Priority %s", static_cast<int>(team), static_cast<int>(roster), has_priority ? "YES" : "NO");
	rootconsole->ConsolePrint("-----------------");
	for (auto& cls : available_classes)
	{
		auto name = tf2lib::GetClassNameFromType(cls);
		rootconsole->ConsolePrint("%s", name);
	}
	rootconsole->ConsolePrint("-----------------");
#endif // EXT_DEBUG


	if (available_classes.size() == 1)
	{
		return available_classes[0];
	}

	// Old method, uniform random
	// return available_classes[randomgen->GetRandomInt<size_t>(0U, available_classes.size() - 1)];

	// New method with weights
	std::vector<int> weights;

	for (auto& cls : available_classes)
	{
		// weight decreases as more players pick this class
		int weight = 10 - (teamdata.GetNumAsClass(cls) * 3);
		weight = std::max(weight, 1);
		weights.push_back(std::move(weight));
	}

#ifdef EXT_DEBUG
	// Sanity check
	if (weights.size() != available_classes.size())
	{
		smutils->LogError(myself, "CTF2ClassSelection::SelectAClass weight and class vector size doesn't match!");
		return TeamFortress2::TFClassType::TFClass_Unknown;
	}
#endif // EXT_DEBUG

	return randomgen->GetWeightedRandom<TeamFortress2::TFClassType, int>(available_classes, weights);
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
		int onteam = UtilHelpers::GetNumberofPlayersOnTeam(static_cast<int>(team));

		if (asclass > data.GetMaximum())
		{
			return true;
		}

		if (data.GetTeamSize() > 0 && onteam < data.GetTeamSize())
		{
			return true;
		}
	}

	return false;
}

bool CTF2ClassSelection::AnyPriorityClass(TeamFortress2::TFTeam team, ClassRosterType roster) const
{
	for (int i = 0; i < static_cast<int>(TeamFortress2::TFClassType::TFClass_Engineer); i++)
	{
		const auto& data = m_classdata[static_cast<int>(roster)][i];
		TeamFortress2::TFClassType current_class = static_cast<TeamFortress2::TFClassType>(i + 1);
		int onteam = UtilHelpers::GetNumberofPlayersOnTeam(static_cast<int>(team));
		int asclass = tf2lib::GetNumberOfPlayersAsClass(current_class, team);
		
		if (data.HasMinimum() && asclass < data.GetMinimum())
		{
			return true;
		}

		if (data.HasRate() && asclass < data.GetMinimumForRate(onteam))
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
		else if (strncasecmp(name, "passtime", 8) == 0)
		{
			m_parserdata.depth = 2;
			m_parserdata.current_roster = ClassRosterType::ROSTER_PASSTIME;
			return SourceMod::SMCResult_Continue;
		}
		else if (strncasecmp(name, "medieval", 8) == 0)
		{
			m_parserdata.depth = 2;
			m_parserdata.current_roster = ClassRosterType::ROSTER_MEDIEVAL;
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
		smutils->LogError(myself, "Out of Bounds write attempt at roster type!");
		return;
	}

	if (c < 0 || c >= static_cast<int>(TeamFortress2::TFClassType::TFClass_Engineer))
	{
		smutils->LogError(myself, "Out of Bounds write attempt at class type!");
		return;
	}

#ifdef EXT_DEBUG
	rootconsole->ConsolePrint("Saving data for class \"%s\" roster %i ", tf2lib::GetClassNameFromType(classtype), r);
	rootconsole->ConsolePrint("%i %i %i %i", data.minimum, data.maximum, data.rate, data.teamsize);
#endif // EXT_DEBUG


	m_classdata[r][c] = data;
}

CTF2ClassSelection::ClassData::ClassData()
{
	minimum = 0;
	maximum = 0;
	rate = 0;
	teamsize = 0;
}
