#include <filesystem>

#include <extension.h>
#include <util/librandom.h>

#include "profile.h"

CDifficultyManager::~CDifficultyManager()
{
	m_profiles.clear();
}

void CDifficultyManager::LoadProfiles()
{
	rootconsole->ConsolePrint("Reading bot difficulty profiles!");

	char path[256]{};

	smutils->BuildPath(SourceMod::Path_SM, path, sizeof(path), "configs/navbot/bot_difficulty.cfg");

	if (std::filesystem::exists(path) == false)
	{
		smutils->LogError(myself, "Failed to read bot difficulty profile configuration file at \"%s\"!", path);
		return;
	}

	SourceMod::SMCStates states;
	auto error = textparsers->ParseFile_SMC(path, this, &states);

	if (error != SourceMod::SMCError_Okay)
	{
		smutils->LogError(myself, "Failed to read bot difficulty profile configuration file at \"%s\"!", path);
		m_profiles.clear();
		return;
	}

	smutils->LogMessage(myself, "Loaded bot difficulty profiles. Number of profiles: %i", m_profiles.size());
}

DifficultyProfile CDifficultyManager::GetProfileForSkillLevel(const int level) const
{
	std::vector<DifficultyProfile> collected;
	collected.reserve(32);

	for (auto& profile : m_profiles)
	{
		if (profile.GetSkillLevel() == level)
		{
			collected.push_back(profile);
		}
	}

	if (collected.size() == 0)
	{
		smutils->LogError(myself, "Difficulty profile for skill level '%i' not found! Using default profile.", level);
		return m_default;
	}

	return collected[librandom::generate_random_uint(0, collected.size() - 1)];
}

void CDifficultyManager::ReadSMC_ParseStart()
{
}

void CDifficultyManager::ReadSMC_ParseEnd(bool halted, bool failed)
{
}

SourceMod::SMCResult CDifficultyManager::ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name)
{
	if (strncmp(name, "BotDifficultyProfiles", 21) == 0)
	{
		m_newprofile = false;
		return SMCResult_Continue;
	}

	// section names are ignored for now
	m_data.OnNew(); // clear it
	m_newprofile = true;
	return SMCResult_Continue;
}

SourceMod::SMCResult CDifficultyManager::ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value)
{
	if (strncasecmp(key, "skill_level", 11) == 0)
	{
		m_data.skill_level = atoi(value);
	}
	else if (strncasecmp(key, "aimspeed", 8) == 0)
	{
		m_data.aimspeed = atof(value);
	}
	else if (strncasecmp(key, "fov", 3) == 0)
	{
		m_data.fov = atoi(value);
	}
	else if (strncasecmp(key, "maxvisionrange", 14) == 0)
	{
		m_data.maxvisionrange = atoi(value);
	}
	else if (strncasecmp(key, "maxhearingrange", 15) == 0)
	{
		m_data.maxhearingrange = atoi(value);
	}
	else
	{
		smutils->LogError(myself, "Unknown key \"%s\" with value \"%s\" found while parsing bot difficulty profile!", key, value);
	}

	return SMCResult_Continue;
}

SourceMod::SMCResult CDifficultyManager::ReadSMC_LeavingSection(const SourceMod::SMCStates* states)
{
	if (m_newprofile == true) // make sure we're not leaving the 'BotDifficultyProfiles' section
	{
		auto& profile = m_profiles.emplace_back();

		profile.SetSkillLevel(m_data.skill_level);
		profile.SetAimSpeed(m_data.aimspeed);
		profile.SetFOV(m_data.fov);
		profile.SetMaxVisionRange(m_data.maxvisionrange);
		profile.SetMaxHearingRange(m_data.maxhearingrange);

		m_newprofile = false; // wait until the next section
	}

	return SMCResult_Continue;
}
