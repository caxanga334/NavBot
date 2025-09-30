#include NAVBOT_PCH_FILE
#include <filesystem>
#include <algorithm>

#include <extension.h>
#include <util/librandom.h>
#include <util/helpers.h>
#include <coordsize.h>
#include "profile.h"

#undef min
#undef max
#undef clamp

void DifficultyProfile::RandomizeProfileValues()
{
	skill_level = SKILL_LEVEL_RANDOM_PROFILE;
	game_awareness = randomgen->GetRandomInt<int>(0, 100);
	weapon_skill = randomgen->GetRandomInt<int>(0, 100);
	aimspeed = randomgen->GetRandomReal<float>(30.0f, 4000.0f);
	fov = randomgen->GetRandomInt<int>(60, 179);
	maxvisionrange = randomgen->GetRandomInt<int>(1024, MAX_COORD_INTEGER);
	maxhearingrange = randomgen->GetRandomInt<int>(256, MAX_COORD_INTEGER / 4); // use sane random hearing distances
	minrecognitiontime = randomgen->GetRandomReal<float>(0.0001f, 1.0f);
	predict_projectiles = randomgen->GetRandomChance(); // GetRandomChance defauls to 50% chance for true
	allow_headshots = randomgen->GetRandomChance();
	can_dodge = randomgen->GetRandomChance();
	aim_tracking_interval = randomgen->GetRandomReal<float>(0.0001f, 1.0f);
	aggressiveness = randomgen->GetRandomInt<int>(0, 100);
	teamwork = randomgen->GetRandomInt<int>(0, 100);
	ability_use_interval = randomgen->GetRandomReal<float>(0.250f, 2.0f);
	health_critical_percent = randomgen->GetRandomReal<float>(0.1f, 0.3f);
	health_low_percent = randomgen->GetRandomReal<float>(0.4f, 0.8f);
	numerical_disadvantage_retreat_threshold = randomgen->GetRandomInt<int>(3, 12);
}

void DifficultyProfile::ValidateProfileValues()
{

	if (health_low_percent <= health_critical_percent)
	{
		smutils->LogError(myself, "Difficulty profile with health low value less than or equals to health critical value!");

		health_low_percent = 0.5f;
		health_critical_percent = 0.2f;
	}
}

CDifficultyManager::~CDifficultyManager()
{
	m_profiles.clear();
}

void CDifficultyManager::LoadProfiles()
{
	rootconsole->ConsolePrint("Reading bot difficulty profiles!");

	std::unique_ptr<char[]> path = std::make_unique<char[]>(PLATFORM_MAX_PATH);

	const char* modfolder = smutils->GetGameFolderName();
	bool found = false;
	bool is_override = false;

	if (modfolder != nullptr)
	{
		// Loof for mod specific custom file
		smutils->BuildPath(SourceMod::Path_SM, path.get(), PLATFORM_MAX_PATH, "configs/navbot/%s/bot_difficulty.custom.cfg", modfolder);

		if (std::filesystem::exists(path.get()))
		{
			found = true;
			is_override = true;
		}
		else
		{
			// look for mod file
			smutils->BuildPath(SourceMod::Path_SM, path.get(), PLATFORM_MAX_PATH, "configs/navbot/%s/bot_difficulty.cfg", modfolder);

			if (std::filesystem::exists(path.get()))
			{
				found = true;
			}
		}
	}

	if (!found) // file not found on mod specific folder
	{
		smutils->BuildPath(SourceMod::Path_SM, path.get(), PLATFORM_MAX_PATH, "configs/navbot/bot_difficulty.custom.cfg");

		if (std::filesystem::exists(path.get()))
		{
			found = true;
			is_override = true;
		}
		else
		{
			smutils->BuildPath(SourceMod::Path_SM, path.get(), PLATFORM_MAX_PATH, "configs/navbot/bot_difficulty.cfg");

			if (std::filesystem::exists(path.get()))
			{
				found = true;
			}
		}
	}

	if (!found)
	{
		smutils->LogError(myself, "Failed to read bot difficulty profile configuration file at \"%s\"!", path.get());
		return;
	}
	else
	{
		if (is_override)
		{
			smutils->LogMessage(myself, "Parsing bot profile override file at \"%s\".", path.get());
		}
	}

	SourceMod::SMCStates states;
	auto error = textparsers->ParseFile_SMC(path.get(), this, &states);

	if (error != SourceMod::SMCError_Okay)
	{
		smutils->LogError(myself, "Failed to read bot difficulty profile configuration file at \"%s\"!", path.get());
		m_profiles.clear();
		return;
	}

	for (auto& profile : m_profiles)
	{
		profile->ValidateProfileValues();
	}

	smutils->LogMessage(myself, "Loaded bot difficulty profiles. Number of profiles: %i", m_profiles.size());
}

std::shared_ptr<DifficultyProfile> CDifficultyManager::GetProfileForSkillLevel(const int level) const
{
	// random profile
	if (level < 0)
	{
		std::shared_ptr<DifficultyProfile> random;
		random.reset(CreateNewProfile());
		random->RandomizeProfileValues();
		return random;
	}

	std::vector<std::shared_ptr<DifficultyProfile>> collected;
	collected.reserve(32);

	for (auto& ptr : m_profiles)
	{
		if (ptr->GetSkillLevel() == level)
		{
			collected.push_back(ptr);
		}
	}

	if (collected.size() == 0)
	{
		smutils->LogError(myself, "Difficulty profile for skill level '%i' not found! Using default profile.", level);
		std::shared_ptr<DifficultyProfile> default_profile;
		default_profile.reset(CreateNewProfile());
		return default_profile;
	}

	return collected[randomgen->GetRandomInt<size_t>(0U, collected.size() - 1U)];
}

void CDifficultyManager::ReadSMC_ParseStart()
{
	m_parser_depth = 0;
	m_current = nullptr;
}

void CDifficultyManager::ReadSMC_ParseEnd(bool halted, bool failed)
{
	m_parser_depth = 0;
	m_current = nullptr;
}

SourceMod::SMCResult CDifficultyManager::ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name)
{
	if (strncmp(name, "BotDifficultyProfiles", 21) == 0)
	{
		m_parser_depth++;
		return SMCResult_Continue;
	}

	if (m_parser_depth == 1)
	{
		// new profile
		auto& profile = m_profiles.emplace_back(CreateNewProfile());
		m_current = profile.get();
	}

	if (m_parser_depth == 2)
	{
		smutils->LogError(myself, "Unexpected section %s at L %i C %i", name, states->line, states->col);
		return SMCResult_HaltFail;
	}

	// max depth should be 2

	if (m_parser_depth > 2)
	{
		smutils->LogError(myself, "Unexpected section %s at L %i C %i", name, states->line, states->col);
		return SMCResult_HaltFail;
	}

	m_parser_depth++;
	return SMCResult_Continue;
}

SourceMod::SMCResult CDifficultyManager::ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value)
{
	if (strncasecmp(key, "skill_level", 11) == 0)
	{
		int v = atoi(value);

		if (v < 0 || v > 255)
		{
			v = 0;
			smutils->LogError(myself, "Skill level should be between 0 and 255! %s <%s> at line %i col %i", key, value, states->line, states->col);
		}

		m_current->SetSkillLevel(v);
	}
	else if (strncasecmp(key, "game_awareness", 14) == 0)
	{
		int v = atoi(value);
		v = std::clamp(v, 0, 100);
		m_current->SetGameAwareness(v);
	}
	else if (strncasecmp(key, "weapon_skill", 12) == 0)
	{
		int v = atoi(value);
		v = std::clamp(v, 0, 100);
		m_current->SetWeaponSkill(v);
	}
	else if (strncasecmp(key, "aimspeed", 8) == 0)
	{
		float v = atof(value);

		if (v < 30.0f)
		{
			v = 31.0f;
			smutils->LogError(myself, "Aim speed cannot be less than 30 degrees per second! %s <%s> at line %i col %i", key, value, states->line, states->col);
		}

		m_current->SetAimSpeed(v);
	}
	else if (strncasecmp(key, "fov", 3) == 0)
	{
		int v = atoi(value);

		if (v < 60 || v > 179)
		{
			v = std::clamp(v, 60, 179);
			smutils->LogError(myself, "FOV should be between 60 and 179! %s <%s> at line %i col %i", key, value, states->line, states->col);
		}

		m_current->SetFOV(v);
	}
	else if (strncasecmp(key, "maxvisionrange", 14) == 0)
	{
		int v = atoi(value);

		if (v < 1024 || v > MAX_COORD_INTEGER)
		{
			v = std::clamp(v, 1024, MAX_COORD_INTEGER);
			smutils->LogError(myself, "Max vision range should be between 1024 and %i! %s <%s> at line %i col %i", MAX_COORD_INTEGER, key, value, states->line, states->col);
		}

		m_current->SetMaxVisionRange(v);
	}
	else if (strncasecmp(key, "maxhearingrange", 15) == 0)
	{
		int v = atoi(value);

		if (v < 256 || v > MAX_COORD_INTEGER)
		{
			v = std::clamp(v, 256, MAX_COORD_INTEGER);
			smutils->LogError(myself, "Max hearing range should be between 256 and %i! %s <%s> at line %i col %i", MAX_COORD_INTEGER, key, value, states->line, states->col);
		}

		m_current->SetMaxHearingRange(v);
	}
	else if (strncasecmp(key, "minrecognitiontime", 18) == 0)
	{
		float v = atof(value);

		if (v < 0.001f || v > 1.0f)
		{
			v = 0.23f;
			smutils->LogError(myself, "Minimum recognition time should be between 0.001 and 1.0! %s <%s> at line %i col %i", key, value, states->line, states->col);
		}

		m_current->SetMinRecognitionTime(v);
	}
	else if (strncasecmp(key, "predict_projectiles", 19) == 0)
	{
		bool predict = UtilHelpers::StringToBoolean(value);
		m_current->SetPredictProjectiles(predict);
	}
	else if (strncasecmp(key, "allow_headshots", 15) == 0)
	{
		bool allow = UtilHelpers::StringToBoolean(value);
		m_current->SetAllowHeadshots(allow);
	}
	else if (strncasecmp(key, "allow_dodging", 13) == 0)
	{
		bool allow = UtilHelpers::StringToBoolean(value);
		m_current->SetIsAllowedToDodge(allow);
	}
	else if (strncasecmp(key, "aim_tracking_interval", 21) == 0)
	{
		float v = atof(value);
		v = std::clamp(v, 0.0f, 1.0f);
		m_current->SetAimTrackingInterval(v);
	}
	else if (strncasecmp(key, "aggressiveness", 14) == 0)
	{
		int v = atoi(value);
		v = std::clamp(v, 0, 100);
		m_current->SetAggressiveness(v);
	}
	else if (strncasecmp(key, "teamwork", 8) == 0)
	{
		int v = atoi(value);
		v = std::clamp(v, 0, 100);
		m_current->SetTeamwork(v);
	}
	else if (strncasecmp(key, "ability_use_interval", 20) == 0)
	{
		float v = atof(value);
		v = std::clamp(v, 0.01f, 2.0f);
		m_current->SetAbilityUsageInterval(v);
	}
	else if (strncasecmp(key, "health_critical_percent", 23) == 0)
	{
		float v = atof(value);
		v = std::clamp(v, 0.01f, 0.6f);
		m_current->SetHealthCriticalThreshold(v);
	}
	else if (strncasecmp(key, "health_low_percent", 18) == 0)
	{
		float v = atof(value);
		v = std::clamp(v, 0.2f, 0.95f);
		m_current->SetHealthLowThreshold(v);
	}
	else if (strncasecmp(key, "numerical_disadvantage_retreat_threshold", 40) == 0)
	{
		int v = atoi(value);
		v = std::clamp(v, 2, 100);
		m_current->SetRetreatFromNumericalDisadvantageThreshold(v);
	}
	else
	{
		smutils->LogError(myself, "Unknown key \"%s\" with value \"%s\" found while parsing bot difficulty profile!", key, value);
	}

	return SMCResult_Continue;
}

SourceMod::SMCResult CDifficultyManager::ReadSMC_LeavingSection(const SourceMod::SMCStates* states)
{
	if (m_parser_depth == 2)
	{
		m_current = nullptr;
	}

	m_parser_depth--;
	return SMCResult_Continue;
}
