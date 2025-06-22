#include <filesystem>
#include <unordered_set>
#include <algorithm>
#include <cstring>

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

	m_index_lookup.clear();
	m_classname_lookup.clear();
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

	for (auto& info : m_weapons)
	{
		info->PostLoad();

		// Item index is set, add to the item index look up map
		if (info->GetItemDefIndex() >= 0)
		{
			auto insert_result = m_index_lookup.insert({ info->GetItemDefIndex(), info.get() });
			
			if (!insert_result.second)
			{
				smutils->LogError(myself, "WeaponInfo: Failed to insert item index %i to look up map! Entry: %s", info->GetItemDefIndex(), info->GetConfigEntryName());
			}
		}
		else
		{
			auto insert_result = m_classname_lookup.insert({ std::string{ info->GetClassname() }, info.get()});

			if (!insert_result.second)
			{
				smutils->LogError(myself, "WeaponInfo: Failed to insert classname %s to look up map! Entry: %s", info->GetClassname(), info->GetConfigEntryName());
			}
		}
	}

	auto functor = [](CBaseBot* bot) {
		bot->GetInventoryInterface()->OnWeaponInfoConfigReloaded();
	};

	extmanager->ForEachBot(functor);

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

const WeaponInfo* CWeaponInfoManager::GetWeaponInfo(std::string classname, const int index) const
{
	const WeaponInfo* result = nullptr;

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
	return m_default.get();
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
	else if (std::strcmp(key, "priority") == 0)
	{
		m_current->SetPriority(atoi(value));
	}
	else if (strncmp(key, "can_headshot", 12) == 0)
	{
		bool v = UtilHelpers::StringToBoolean(value);
		m_current->SetCanHeadShot(v);
	}
	else if (strncmp(key, "infinite_reserve_ammo", 13) == 0)
	{
		bool v = UtilHelpers::StringToBoolean(value);
		m_current->SetInfiniteReserveAmmo(v);
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
	else if (std::strcmp(key, "primary_no_clip") == 0)
	{
		m_current->SetNoClip1(true);
	}
	else if (std::strcmp(key, "secondary_no_clip") == 0)
	{
		m_current->SetNoClip2(true);
	}
	else if (std::strcmp(key, "secondary_uses_primary_ammo_type") == 0)
	{
		m_current->SetSecondaryUsesPrimaryAmmoType(true);
	}
	else if (strncmp(key, "low_primary_ammo_threshold", 26) == 0)
	{
		m_current->SetLowPrimaryAmmoThreshold(atoi(value));
	}
	else if (strncmp(key, "low_secondary_ammo_threshold", 28) == 0)
	{
		m_current->SetLowSecondaryAmmoThreshold(atoi(value));
	}
	else if (std::strcmp(key, "slot") == 0)
	{
		m_current->SetSlot(atoi(value));
	}
	else if (strncmp(key, "attack_interval", 15) == 0)
	{
		float v = atof(value);
		
		if (v <= 0.0f)
		{
			v = -1.0f;
		}

		m_current->SetAttackInterval(v);

	}
	else if (strncmp(key, "attack_range_override", 21) == 0)
	{
		float flvalue = atof(value);
		m_current->SetAttackRange(flvalue);
	}
	else if (strncmp(key, "use_secondary_attack_chance", 27) == 0)
	{
		int v = atoi(value);
		v = std::clamp(v, 1, 100);
		m_current->SetChanceToUseSecondaryAttack(v);
	}
	else if (strncmp(key, "custom_ammo_property_name", 25) == 0)
	{
		m_current->SetCustomAmmoPropertyName(value);
	}
	else if (strncmp(key, "custom_ammo_property_source", 27) == 0)
	{
		if (strncmp(value, "player", 6) == 0)
		{
			m_current->SetCustomAmmoPropertySource(false);
		}
		else
		{
			m_current->SetCustomAmmoPropertySource(true);
		}
	}
	else if (strncmp(key, "custom_ammo_property_type", 27) == 0)
	{
		if (strncmp(value, "networked", 9) == 0)
		{
			m_current->SetCustomAmmoPropertyType(true);
		}
		else
		{
			m_current->SetCustomAmmoPropertyType(false);
		}
	}
	else if (strncmp(key, "custom_ammo_property_out_of_ammo_threshold", 42) == 0)
	{
		float v = atof(value);
		m_current->SetCustomAmmoOutOfAmmoThreshold(v);
	}
	else if (strncmp(key, "custom_ammo_property_is_float", 29) == 0)
	{
		bool v = UtilHelpers::StringToBoolean(value);
		m_current->SetCustomAmmoPropertyIsFloat(v);
	}
	else if (std::strcmp(key, "priority_dynamic_has_secondary_ammo") == 0)
	{
		int v = atoi(value);
		m_current->SetDynamicPriorityHasSecondaryAmmo(v);
	}
	else if (std::strcmp(key, "priority_dynamic_health_percentage") == 0)
	{
		int v = atoi(value);
		m_current->SetDynamicPriorityHealthPercentage(v);
	}
	else if (std::strcmp(key, "priority_dynamic_health_percentage_threshold") == 0)
	{
		float v = atof(value);
		m_current->SetDynamicPriorityHealthPercentageCondition(v);
	}
	else if (std::strcmp(key, "priority_dynamic_threat_range_less_than") == 0)
	{
		int v = atoi(value);
		m_current->SetDynamicPriorityThreatRangeLessThan(v);
	}
	else if (std::strcmp(key, "priority_dynamic_threat_range_less_than_threshold") == 0)
	{
		float v = atof(value);
		m_current->SetDynamicPriorityThreatRangeLessThanCondition(v);
	}
	else if (std::strcmp(key, "deployed_property_name") == 0)
	{
		m_current->SetDeployedPropertyName(value);
	}
	else if (std::strcmp(key, "needs_to_be_deployed_to_fire") == 0)
	{
		bool v = UtilHelpers::StringToBoolean(value);
		m_current->SetNeedsToBeDeployed(v);
	}
	else if (std::strcmp(key, "deployed_property_source") == 0)
	{
		if (std::strcmp(value, "player") == 0)
		{
			m_current->SetDeployedPropertySource(false);
		}
		else
		{
			m_current->SetDeployedPropertySource(true);
		}
	}
	else if (std::strcmp(key, "disable_dodge") == 0)
	{
		bool v = UtilHelpers::StringToBoolean(value);
		m_current->SetIsDodgeDisabled(v);
	}
	else if (std::strcmp(key, "selection_max_range_override") == 0)
	{
		float v = atof(value);
		m_current->SetSelectionMaxRangeOverride(v);
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
		else if (std::strcmp(key, "projectilespeed") == 0)
		{
			m_current->GetAttackInfoForEditing(type)->SetProjectileSpeed(atof(value));
		}
		else if (std::strcmp(key, "gravity") == 0)
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
		else if (strncmp(key, "hold_button_time", 16) == 0)
		{
			float v = atof(value);

			if (v <= 0.0f)
			{
				v = -1.0f;
			}

			m_current->GetAttackInfoForEditing(type)->SetHoldButtonTime(v);
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

void WeaponInfo::PostLoad()
{
	// If not set by the config file, use the largest distance between all available attacks.
	if (attack_move_range <= 0.0f)
	{
		for (size_t i = 0; i < static_cast<size_t>(MAX_WEAPON_ATTACKS); i++)
		{
			if (attacksinfo[i].HasFunction() && attacksinfo[i].GetMaxRange() > attack_move_range)
			{
				attack_move_range = attacksinfo[i].GetMaxRange();
			}
		}
	}

	// not set by the config file
	if (selection_max_range <= 0.0f)
	{
		float minrange = 9999999999.0f;

		for (size_t i = 0; i < static_cast<size_t>(MAX_WEAPON_ATTACKS); i++)
		{
			if (attacksinfo[i].HasFunction())
			{
				if (attacksinfo[i].GetMaxRange() > selection_max_range)
				{
					selection_max_range = attacksinfo[i].GetMaxRange();
				}

				if (attacksinfo[i].GetMinRange() > 0.0f && attacksinfo[i].GetMinRange() < minrange)
				{
					minrange = attacksinfo[i].GetMinRange();
				}
			}
		}

		if (minrange < selection_max_range)
		{
			selection_min_range = minrange;
		}
		else
		{
			selection_min_range = -1.0f;
		}
	}
}

CON_COMMAND(sm_navbot_reload_weaponinfo_config, "Reloads the weapon info configuration file.")
{
	extmanager->GetMod()->ReloadWeaponInfoConfigFile();
}