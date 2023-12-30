#include <filesystem>
#include <unordered_set>
#include <algorithm>

#include <extension.h>

#include "weaponinfo.h"

#undef min
#undef max
#undef clamp

bool CWeaponInfoManager::LoadConfigFile()
{
	char fullpath[PLATFORM_MAX_PATH];
	auto gamefolder = smutils->GetGameFolderName();
	smutils->BuildPath(SourceMod::Path_SM, fullpath, sizeof(fullpath), "configs/navbot/%s/weapons.cfg", gamefolder);

	if (!std::filesystem::exists(fullpath))
	{
		smutils->LogError(myself, "Failed to load Weapon Info configuration file \"%s\". File does not exists!", fullpath);
		return false;
	}

	InitParserData();
	SourceMod::SMCStates state;
	auto errorcode = textparsers->ParseFile_SMC(fullpath, this, &state);

	if (errorcode != SourceMod::SMCError::SMCError_Okay)
	{
		smutils->LogError(myself, "Failed to parse Weapon Info configuration file \"%s\". Parser received error %i (%s)", 
			fullpath, static_cast<int>(errorcode), textparsers->GetSMCErrorString(errorcode));

		return false;
	}

	PostParseAnalysis();

	return true;
}

void CWeaponInfoManager::PostParseAnalysis()
{
	std::unordered_set<std::string> entries;
	std::unordered_set<int> itemindexes;
	entries.reserve(m_weapons.size());

	for (auto& weaponinfo : m_weapons)
	{
		std::string entry(weaponinfo.GetConfigEntryName());

		// duplicate check
		if (entries.find(entry) == entries.end())
		{
			entries.emplace(weaponinfo.GetConfigEntryName());
		}
		else
		{
			smutils->LogError(myself, "Duplicate Weapon Info entry found! \"%s\" ", entry.c_str());
		}

		auto index = weaponinfo.GetItemDefIndex();

		if (index >= 0)
		{
			if (itemindexes.find(index) == itemindexes.end())
			{
				itemindexes.emplace(index);
			}
			else
			{
				smutils->LogError(myself, "Duplicate Weapon Info Item Definition Index found! \"%i\" ", index);
			}
		}

		auto classname = weaponinfo.GetClassname();

		// check if classname is missing by length, most weapons should have a 'weapon' somewhere in their classname
		if (strlen(classname) < 3)
		{
			smutils->LogError(myself, "Weapon Info entry with missing classname! \"%s\" ", entry.c_str());
		}
	}
}

SMCResult CWeaponInfoManager::ReadSMC_NewSection(const SMCStates* states, const char* name)
{
	// If the file doesn't start with this, the file is invalid
	if (strncmp(name, "WeaponInfoConfig", 16) == 0)
	{
		m_isvalid = true;
		return SMCResult_Continue;
	}

	if (!m_isvalid)
	{
		return SMCResult_Halt;
	}

	// check weapon info sections
	if (strncmp(name, "primary_attack_info", 19) == 0)
	{
		m_section_prim = true;

		if (!m_section_weapon)
		{
			return SMCResult_Halt;
		}
	}
	else if (strncmp(name, "secondary_attack_info", 21) == 0)
	{
		m_section_sec = true;

		if (!m_section_weapon)
		{
			return SMCResult_Halt;
		}
	}
	else if (strncmp(name, "tertiary_attack_info", 20) == 0)
	{
		m_section_ter = true;

		if (!m_section_weapon)
		{
			return SMCResult_Halt;
		}
	}

	if (!m_section_weapon) // weapon section can be anything
	{
		m_section_weapon = true;
		m_tempweapinfo.Reset();
		m_tempweapinfo.SetConfigEntryName(name);
	}
	else if (IsParserInWeaponAttackSection())
	{
		return SMCResult_Continue;
	}
	else // not a weapon entry section and not a know section name
	{
		smutils->LogError(myself, "Unknown section! %s", name);
		return SMCResult_Halt;
	}

	return SMCResult_Continue;
}

SMCResult CWeaponInfoManager::ReadSMC_KeyValue(const SMCStates* states, const char* key, const char* value)
{
	if (strncmp(key, "classname", 9) == 0)
	{
		m_tempweapinfo.SetClassname(value);
	}
	else if (strncmp(key, "itemindex", 9) == 0)
	{
		m_tempweapinfo.SetEconItemIndex(atoi(value));
	}
	else if (strncmp(key, "priority", 8) == 0)
	{
		m_tempweapinfo.SetPriority(atoi(value));
	}
	else if (strncmp(key, "can_headshot", 12) == 0)
	{
		if (strncmp(value, "true", 4) == 0)
		{
			m_tempweapinfo.SetCanHeadShot(true);
		}
		else
		{
			m_tempweapinfo.SetCanHeadShot(false);
		}
	}
	else if (strncmp(key, "headshot_range_multiplier", 25) == 0)
	{
		float hsrange = atof(value);
		hsrange = std::clamp(hsrange, 0.0f, 1.0f);
		m_tempweapinfo.SetHeadShotRangeMultiplier(hsrange);
	}

	if (IsParserInWeaponAttackSection())
	{
		WeaponInfo::AttackFunctionType type = WeaponInfo::PRIMARY_ATTACK;

		if (m_section_sec)
		{
			type = WeaponInfo::SECONDARY_ATTACK;
		}
		else if (m_section_ter)
		{
			type = WeaponInfo::TERTIARY_ATTACK;
		}

		if (strncmp(key, "maxrange", 8) == 0)
		{
			m_tempweapinfo.GetAttackInfoForEditing(type)->SetMaxRange(atof(value));
		}
		else if (strncmp(key, "minrange", 8) == 0)
		{
			m_tempweapinfo.GetAttackInfoForEditing(type)->SetMinRange(atof(value));
		}
		else if (strncmp(key, "projectilespeed", 15) == 0)
		{
			m_tempweapinfo.GetAttackInfoForEditing(type)->SetProjectileSpeed(atof(value));
		}
		else if (strncmp(key, "melee", 5) == 0)
		{
			if (strncmp(value, "true", 4) == 0)
			{
				m_tempweapinfo.GetAttackInfoForEditing(type)->SetMelee(true);
			}
			else
			{
				m_tempweapinfo.GetAttackInfoForEditing(type)->SetMelee(false);
			}
		}
		else if (strncmp(key, "explosive", 7) == 0)
		{
			if (strncmp(value, "true", 4) == 0)
			{
				m_tempweapinfo.GetAttackInfoForEditing(type)->SetExplosive(true);
			}
			else
			{
				m_tempweapinfo.GetAttackInfoForEditing(type)->SetExplosive(false);
			}
		}
	}

	return SMCResult_Continue;
}

SMCResult CWeaponInfoManager::ReadSMC_LeavingSection(const SMCStates* states)
{
	if (m_section_weapon)
	{
		if (IsParserInWeaponAttackSection())
		{
			ParserExitWeaponSection();
			return SMCResult_Continue;
		}

		m_weapons.push_back(m_tempweapinfo);
		m_section_weapon = false;
	}

	return SMCResult_Continue;
}
