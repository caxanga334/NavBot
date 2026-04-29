#include NAVBOT_PCH_FILE
#include "css_mod.h"
#include "css_buy_profile.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

counterstrikesource::BuyManager::BuyManager()
{
	m_current_profile = nullptr;
	m_parser_section = 0;
}

bool counterstrikesource::BuyManager::ParseConfigFile()
{
	CCounterStrikeSourceMod* cssmod = CCounterStrikeSourceMod::GetCSSMod();
	std::string file;
	
	if (!cssmod->BuildPathToModFile("configs/navbot", CONFIG_FILE_NAME, USER_CONFIG_FILE_NAME, file))
	{
		smutils->LogError(myself, "Could not parse buy profiles config file!");
		return false;
	}

	m_buyprofiles.clear();
	m_prices.clear();
	m_teamrestrictions.clear();

	SourceMod::SMCStates states;
	auto error = textparsers->ParseFile_SMC(file.c_str(), this, &states);

	if (error != SourceMod::SMCError::SMCError_Okay)
	{
		smutils->LogError(myself, "Failed to parse buy profiles config file! Error: %s", textparsers->GetSMCErrorString(error));
		return false;
	}

	return true;
}

const counterstrikesource::BuyProfile* counterstrikesource::BuyManager::GetRandomBuyProfile() const
{
	if (m_buyprofiles.empty()) { return nullptr; }

	std::size_t idx = randomgen->GetRandomInt<std::size_t>(0U, m_buyprofiles.size() - 1U);
	return &m_buyprofiles[idx];
}

int counterstrikesource::BuyManager::LookupPrice(const char* name) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("BuyManager::LookupPrice", "NavBot");
#endif // EXT_VPROF_ENABLED

	for (auto& price : m_prices)
	{
		if (std::strcmp(price.first.c_str(), name) == 0)
		{
			return price.second;
		}
	}

	return std::numeric_limits<int>::max();
}

bool counterstrikesource::BuyManager::IsAvailableToTeam(const char* name, const counterstrikesource::CSSTeam team) const
{
	for (auto& entry : m_teamrestrictions)
	{
		if (std::strcmp(entry.first.c_str(), name) == 0)
		{
			return entry.second == static_cast<int>(team);
		}
	}

	return true;
}

void counterstrikesource::BuyManager::ReadSMC_ParseStart()
{
	m_current_profile = nullptr;
	m_parser_section = SECTION_INIT;
}

SourceMod::SMCResult counterstrikesource::BuyManager::ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name)
{

	switch (m_parser_section)
	{
	case SECTION_INIT:
	{
		if (std::strcmp(name, "BuyInfo") == 0)
		{
			return SourceMod::SMCResult::SMCResult_Continue;
		}

		if (std::strcmp(name, "profiles") == 0)
		{
			m_parser_section = SECTION_PROFILES;
			return SourceMod::SMCResult::SMCResult_Continue;
		}

		if (std::strcmp(name, "prices") == 0)
		{
			m_parser_section = SECTION_PRICES;
			return SourceMod::SMCResult::SMCResult_Continue;
		}

		if (std::strcmp(name, "team_restrictions") == 0)
		{
			m_parser_section = SECTION_TEAM_RESTRICTIONS;
			return SourceMod::SMCResult::SMCResult_Continue;
		}

		break;
	}
	case SECTION_PROFILES:
	{
		m_current_profile = AddNewProfile(name);
		m_parser_section = SECTION_PROFILE;
		return SourceMod::SMCResult::SMCResult_Continue;
	}
	case SECTION_PROFILE:
	{
		if (std::strcmp(name, "primaries") == 0)
		{
			m_parser_section = SECTION_PRIMARIES;
			return SourceMod::SMCResult::SMCResult_Continue;
		}

		if (std::strcmp(name, "secondaries") == 0)
		{
			m_parser_section = SECTION_SECONDARIES;
			return SourceMod::SMCResult::SMCResult_Continue;
		}

		if (std::strcmp(name, "options") == 0)
		{
			m_parser_section = SECTION_OPTIONS;
			return SourceMod::SMCResult::SMCResult_Continue;
		}

		break;
	}
	default:
		break;
	}

	smutils->LogError(myself, "Unknown section \"%s\" at line %u col %u!", name, states->line, states->col);
	return SourceMod::SMCResult::SMCResult_HaltFail;
}

SourceMod::SMCResult counterstrikesource::BuyManager::ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value)
{
	switch (m_parser_section)
	{
	case SECTION_PRICES:
		
		if (!AddPrice(key, atoi(value)))
		{
			smutils->LogError(myself, "Duplicated price entry \"%s\" at line %u col %u!", key, states->line, states->col);
		}

		return SourceMod::SMCResult::SMCResult_Continue;
	case SECTION_PRIMARIES:
		m_current_profile->AddPrimary(key);
		return SourceMod::SMCResult::SMCResult_Continue;
	case SECTION_SECONDARIES:
		m_current_profile->AddSecondary(key);
		return SourceMod::SMCResult::SMCResult_Continue;
	case SECTION_OPTIONS:
		if (!m_current_profile->ParseOption(key, value))
		{
			break;
		}

		return SourceMod::SMCResult::SMCResult_Continue;
	case SECTION_TEAM_RESTRICTIONS:

		if (!AddTeamRestricion(key, value))
		{
			smutils->LogError(myself, "Failed to add team restriction entry (%s:%s) at line %u col %u!", key, value, states->line, states->col);
		}

		return SourceMod::SMCResult::SMCResult_Continue;
	default:
		break;
	}


	smutils->LogError(myself, "Bad key value pair \"%s:%s\" at line %u col %u!", key, value, states->line, states->col);
	return SourceMod::SMCResult::SMCResult_HaltFail;
}

SourceMod::SMCResult counterstrikesource::BuyManager::ReadSMC_LeavingSection(const SourceMod::SMCStates* states)
{
	switch (m_parser_section)
	{
	case SECTION_TEAM_RESTRICTIONS:
		[[fallthrough]];
	case SECTION_PRICES:
		[[fallthrough]];
	case SECTION_PROFILES:
		m_parser_section = SECTION_INIT;
		return SourceMod::SMCResult::SMCResult_Continue;
	case SECTION_PROFILE:
		m_parser_section = SECTION_PROFILES;
		return SourceMod::SMCResult::SMCResult_Continue;
	case SECTION_PRIMARIES:
		[[fallthrough]];
	case SECTION_SECONDARIES:
		[[fallthrough]];
	case SECTION_OPTIONS:
		m_parser_section = SECTION_PROFILE;
		return SourceMod::SMCResult::SMCResult_Continue;
	case SECTION_INIT:
		return SourceMod::SMCResult::SMCResult_Continue;
	default:
		break;
	}

	smutils->LogError(myself, "Malformed file at line %u col %u!", states->line, states->col);
	return SourceMod::SMCResult::SMCResult_HaltFail;
}

counterstrikesource::BuyProfile* counterstrikesource::BuyManager::AddNewProfile(const char* name)
{
	for (auto& profile : m_buyprofiles)
	{
		if (std::strcmp(profile.m_name.c_str(), name) == 0)
		{
			return &profile;
		}
	}

	BuyProfile& profile = m_buyprofiles.emplace_back();
	profile.m_name.assign(name);
	return &profile;
}

bool counterstrikesource::BuyManager::AddPrice(const char* name, int price)
{
	for (auto& price : m_prices)
	{
		if (std::strcmp(price.first.c_str(), name) == 0)
		{
			return false;
		}
	}

	m_prices.emplace_back(name, price);
	return true;
}

bool counterstrikesource::BuyManager::AddTeamRestricion(const char* key, const char* value)
{
	for (auto& entry : m_teamrestrictions)
	{
		if (std::strcmp(entry.first.c_str(), key) == 0)
		{
			return false;
		}
	}

	int team = ParseTeamName(value);

	if (team <= TEAM_SPECTATOR)
	{
		return false;
	}

	m_teamrestrictions.emplace_back(key, team);
	return true;
}

int counterstrikesource::BuyManager::ParseTeamName(const char* name)
{
	if (std::strcmp(name, "TEAM_T") == 0)
	{
		return static_cast<int>(counterstrikesource::CSSTeam::TERRORIST);
	}

	if (std::strcmp(name, "TEAM_CT") == 0)
	{
		return static_cast<int>(counterstrikesource::CSSTeam::COUNTERTERRORIST);
	}

	return static_cast<int>(counterstrikesource::CSSTeam::UNASSIGNED);
}

bool counterstrikesource::BuyProfile::ParseOption(const char* key, const char* value)
{
	if (std::strcmp(key, "defuser_chance") == 0)
	{
		m_defuserchance = std::clamp(atoi(value), 0, 100);
		return true;
	}

	if (std::strcmp(key, "min_money_for_nades") == 0)
	{
		m_minmoneyfornades = std::clamp(atoi(value), 1000, 15000);
		return true;
	}

	if (std::strcmp(key, "min_money_for_sec") == 0)
	{
		m_minmoneyforsecupgrade = std::clamp(atoi(value), 1000, 15000);
		return true;
	}

	if (std::strcmp(key, "he_chance") == 0)
	{
		m_hechance = std::clamp(atoi(value), 0, 100);
		return true;
	}

	if (std::strcmp(key, "flash_chance") == 0)
	{
		m_flashchance = std::clamp(atoi(value), 0, 100);
		return true;
	}

	if (std::strcmp(key, "smoke_chance") == 0)
	{
		m_smokechance = std::clamp(atoi(value), 0, 100);
		return true;
	}

	return false;
}
