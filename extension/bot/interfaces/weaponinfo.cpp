#include <filesystem>
#include <unordered_set>
#include <algorithm>

#include <extension.h>
#include <manager.h>
#include <mods/basemod.h>
#include <bot/basebot.h>

#include "weaponinfo.h"

#undef min
#undef max
#undef clamp

bool CWeaponInfoManager::LoadConfigFile()
{
	std::unique_ptr<char[]> path = std::make_unique<char[]>(PLATFORM_MAX_PATH);

	auto gamefolder = smutils->GetGameFolderName();
	smutils->BuildPath(SourceMod::Path_SM, path.get(), PLATFORM_MAX_PATH, "configs/navbot/%s/weapons.custom.cfg", gamefolder);

	if (!std::filesystem::exists(path.get()))
	{
		smutils->BuildPath(SourceMod::Path_SM, path.get(), PLATFORM_MAX_PATH, "configs/navbot/%s/weapons.cfg", gamefolder);

		if (!std::filesystem::exists(path.get()))
		{
			smutils->LogError(myself, "Failed to load Weapon Info configuration file \"%s\". File does not exists!", path.get());
			return false;
		}
	}

	m_weapons.clear();

	InitParserData();
	SourceMod::SMCStates state;
	auto errorcode = textparsers->ParseFile_SMC(path.get(), this, &state);

	if (errorcode != SourceMod::SMCError::SMCError_Okay)
	{
		smutils->LogError(myself, "Failed to parse Weapon Info configuration file \"%s\". Parser received error %i (%s)", 
			path.get(), static_cast<int>(errorcode), textparsers->GetSMCErrorString(errorcode));

		return false;
	}

	PostParseAnalysis();

	extmanager->ForEachBot([](CBaseBot* bot) {
		bot->GetInventoryInterface()->OnWeaponInfoConfigReloaded();
	});

	return true;
}

void CWeaponInfoManager::PostParseAnalysis()
{
	std::unordered_set<std::string> entries;
	std::unordered_set<int> itemindexes;
	entries.reserve(m_weapons.size());

	for (auto& weaponinfoptr : m_weapons)
	{
		std::string entry(weaponinfoptr->GetConfigEntryName());

		// duplicate check
		if (entries.find(entry) == entries.end())
		{
			entries.emplace(weaponinfoptr->GetConfigEntryName());
		}
		else
		{
			smutils->LogError(myself, "Duplicate Weapon Info entry found! \"%s\" ", entry.c_str());
		}

		auto index = weaponinfoptr->GetItemDefIndex();

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

		auto classname = weaponinfoptr->GetClassname();

		// check if classname is missing by length, most weapons should have a 'weapon' somewhere in their classname
		if (strlen(classname) < 3)
		{
			smutils->LogError(myself, "Weapon Info entry with missing classname! \"%s\" ", entry.c_str());
		}
	}

	// was on the constructor, moved here so the default is created using the derived class type
	m_default.reset(CreateWeaponInfo());
}

std::shared_ptr<WeaponInfo> CWeaponInfoManager::GetWeaponInfo(std::string classname, const int index) const
{
	std::shared_ptr<WeaponInfo> result{ nullptr };

	result = LookUpWeaponInfoByEconIndex(index);

	if (result)
	{
		return result;
	}

	result = LookUpWeaponInfoByClassname(classname);

	if (result)
	{
		return result;
	}

	smutils->LogError(myself, "CWeaponInfoManager::GetWeaponInfo Failed to find WeaponInfo for %s <%i>", classname.c_str(), index);
	return m_default;
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
		auto& newinfo = m_weapons.emplace_back(CreateWeaponInfo());
		m_current = newinfo.get();
		m_current->SetConfigEntryName(name);
	}
	else if (IsParserInWeaponAttackSection())
	{
		return SMCResult_Continue;
	}
	else // not a weapon entry section and not a know section name
	{
		smutils->LogError(myself, "Unknown section! %s at line %d col %d", name, states->line, states->col);
		return SMCResult_Halt;
	}

	return SMCResult_Continue;
}

SMCResult CWeaponInfoManager::ReadSMC_KeyValue(const SMCStates* states, const char* key, const char* value)
{
	if (strncmp(key, "classname", 9) == 0)
	{
		m_current->SetClassname(value);
	}
	else if (strncmp(key, "itemindex", 9) == 0)
	{
		m_current->SetEconItemIndex(atoi(value));
	}
	else if (strncmp(key, "priority", 8) == 0)
	{
		m_current->SetPriority(atoi(value));
	}
	else if (strncmp(key, "can_headshot", 12) == 0)
	{
		if (strncmp(value, "true", 4) == 0)
		{
			m_current->SetCanHeadShot(true);
		}
		else
		{
			m_current->SetCanHeadShot(false);
		}
	}
	else if (strncmp(key, "headshot_range_multiplier", 25) == 0)
	{
		float hsrange = atof(value);
		hsrange = std::clamp(hsrange, 0.0f, 1.0f);
		m_current->SetHeadShotRangeMultiplier(hsrange);
	}
	else if (strncmp(key, "headshot_aim_offset", 19) == 0)
	{
		Vector offset;
		int num_found = sscanf(value, "%f %f %f", &offset.x, &offset.y, &offset.z);

		if (num_found == 3)
		{
			m_current->SetHeadShotAimOffset(offset);
		}
	}
	else if (strncmp(key, "maxclip1", 8) == 0)
	{
		m_current->SetMaxClip1(atoi(value));
	}
	else if (strncmp(key, "maxclip2", 8) == 0)
	{
		m_current->SetMaxClip2(atoi(value));
	}
	else if (strncmp(key, "low_primary_ammo_threshold", 26) == 0)
	{
		m_current->SetLowPrimaryAmmoThreshold(atoi(value));
	}
	else if (strncmp(key, "low_secondary_ammo_threshold", 28) == 0)
	{
		m_current->SetLowSecondaryAmmoThreshold(atoi(value));
	}
	else if (strncmp(key, "slot", 4) == 0)
	{
		m_current->SetSlot(atoi(value));
	}
	else if (!IsParserInWeaponAttackSection())
	{
		smutils->LogError(myself, "[WEAPON INFO PARSER] Unknown key value pair <%s - %s> at line %i col %i", key, value, states->line, states->col);
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
			m_current->GetAttackInfoForEditing(type)->SetMaxRange(atof(value));
		}
		else if (strncmp(key, "minrange", 8) == 0)
		{
			m_current->GetAttackInfoForEditing(type)->SetMinRange(atof(value));
		}
		else if (strncmp(key, "projectilespeed", 15) == 0)
		{
			m_current->GetAttackInfoForEditing(type)->SetProjectileSpeed(atof(value));
		}
		else if (strncmp(key, "gravity", 7) == 0)
		{
			m_current->GetAttackInfoForEditing(type)->SetGravity(atof(value));
		}
		else if (strncmp(key, "ballistic_elevation_range_start", 31) == 0)
		{
			m_current->GetAttackInfoForEditing(type)->SetBallisticElevationStartRange(atof(value));
		}
		else if (strncmp(key, "ballistic_elevation_range_end", 30) == 0)
		{
			m_current->GetAttackInfoForEditing(type)->SetBallisticElevationEndRange(atof(value));
		}
		else if (strncmp(key, "ballistic_elevation_min", 23) == 0)
		{
			m_current->GetAttackInfoForEditing(type)->SetBallisticElevationMin(atof(value));
		}
		else if (strncmp(key, "ballistic_elevation_max", 23) == 0)
		{
			m_current->GetAttackInfoForEditing(type)->SetBallisticElevationMax(atof(value));
		}
		else if (strncmp(key, "melee", 5) == 0)
		{
			if (strncmp(value, "true", 4) == 0)
			{
				m_current->GetAttackInfoForEditing(type)->SetMelee(true);
			}
			else
			{
				m_current->GetAttackInfoForEditing(type)->SetMelee(false);
			}
		}
		else if (strncmp(key, "explosive", 7) == 0)
		{
			if (strncmp(value, "true", 4) == 0)
			{
				m_current->GetAttackInfoForEditing(type)->SetExplosive(true);
			}
			else
			{
				m_current->GetAttackInfoForEditing(type)->SetExplosive(false);
			}
		}
		else
		{
			smutils->LogError(myself, "[WEAPON INFO PARSER] Unknown key value pair (Attack Info %i) <%s - %s> at line %i col %i", 
				static_cast<int>(type), key, value, states->line, states->col);
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

		m_section_weapon = false;
	}

	return SMCResult_Continue;
}

CON_COMMAND(sm_navbot_reload_weaponinfo_config, "Reloads the weapon info configuration file.")
{
	extmanager->GetMod()->ReloadWeaponInfoConfigFile();
}