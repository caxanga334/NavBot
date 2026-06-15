#include NAVBOT_PCH_FILE
#include <mods/basemod.h>
#include <bot/basebot.h>
#include "weapons/dynamic_priority_manager.h"
#include "weaponinfo.h"

#undef min
#undef max
#undef clamp

bool WeaponEntityProperty::SetupFromString(const char* str)
{
	if (ke::StrCaseCmp(str, "reset") == 0)
	{
		Reset();
		return true;
	}

	std::size_t count = 0U;
	std::string szValue(str);
	std::stringstream stream(szValue);
	std::string token;

	// format: prop name, prop type, value type, compare type, on player, value 1, value 2 (optional)

	while (std::getline(stream, token, ','))
	{
		switch (count)
		{
		case 0U:
		{
			m_propName = token;
			// unlikely for a property name to be this small
			if (m_propName.length() <= 2) { return false; }
			break;
		}
		case 1U:
		{
			SetPropType(entityprops::StringToPropType(token.c_str()));
			break;
		}
		case 2U:
		{
			SetValueType(GetValueFromString(token));
			if (m_valueType == ValueType::TYPE_UNKNOWN) { return false; }
			break;
		}
		case 3U:
		{
			SetCompareType(GetCompareTypeFromString(token));
			if (m_compareType == CompareType::COMPARE_NONE) { return false; }
			break;
		}
		case 4U:
		{
			SetOnPlayer(UtilHelpers::StringToBoolean(token.c_str()));
			break;
		}
		case 5U:
		{
			if (!ReadValue1(token)) { return false; }
			break;
		}
		case 6U:
		{
			if (!ReadValue2(token)) { return false; }
			break;
		}
		default:
			break;
		}

		count++;
	}

	if (!IsValid())
	{
		return false;
	}

	return true;
}

bool WeaponEntityProperty::TestCondition(CBaseEntity* player, CBaseEntity* weapon) const
{
	if (!IsValid()) { return false; }

	try
	{
		switch (m_valueType)
		{
		case ValueType::TYPE_BOOL:
		{
			return TestBoolean(player, weapon);
		}
		case ValueType::TYPE_INT:
		{
			return TestInt(player, weapon);
		}
		case ValueType::TYPE_FLOAT:
		{
			return TestFloat(player, weapon);
		}
		default:
			return false;
		}
	}
	catch (const std::bad_any_cast& ex)
	{
#ifdef EXT_DEBUG
		META_CONPRINTF("[NavBot] WeaponEntityProperty::TestCondition C++ Exception! %s \n", ex.what());
#endif // EXT_DEBUG
		return false;
	}
}

int WeaponEntityProperty::ReadPropBool(CBaseEntity* player, CBaseEntity* weapon) const
{
	if (m_valueType != ValueType::TYPE_BOOL) { return false; }

	bool value = 0;

	if (!entprops->GetEntPropBool(IsOnPlayer() ? player : weapon, GetPropType(), GetPropertyName().c_str(), value))
	{
		return false;
	}

	return value;
}

int WeaponEntityProperty::ReadPropInt(CBaseEntity* player, CBaseEntity* weapon) const
{
	if (m_valueType != ValueType::TYPE_INT) { return 0; }

	int value = 0;

	if (!entprops->GetEntProp(IsOnPlayer() ? player : weapon, GetPropType(), GetPropertyName().c_str(), value))
	{
		return 0;
	}

	return value;
}

float WeaponEntityProperty::ReadPropFloat(CBaseEntity* player, CBaseEntity* weapon) const
{
	if (m_valueType != ValueType::TYPE_FLOAT) { return 0.0f; }

	float value = 0;

	if (!entprops->GetEntPropFloat(IsOnPlayer() ? player : weapon, GetPropType(), GetPropertyName().c_str(), value))
	{
		return 0.0f;
	}

	return value;
}

bool WeaponEntityProperty::ReadValue1(const std::string& str)
{
	try
	{
		switch (m_valueType)
		{
		case ValueType::TYPE_BOOL:
		{
			SetValue1(UtilHelpers::StringToBoolean(str.c_str()));
			break;
		}
		case ValueType::TYPE_INT:
		{
			int value = std::stoi(str);
			SetValue1(value);
			break;
		}
		case ValueType::TYPE_FLOAT:
		{
			float value = std::stof(str);
			SetValue1(value);
			break;
		}
		default:
			return false;
		}
	}
	catch (const std::invalid_argument&)
	{
		return false;
	}
	catch (const std::out_of_range&)
	{
		return false;
	}

	return true;
}

bool WeaponEntityProperty::ReadValue2(const std::string& str)
{
	try
	{
		switch (m_valueType)
		{
		case ValueType::TYPE_BOOL:
		{
			SetValue2(UtilHelpers::StringToBoolean(str.c_str()));
			break;
		}
		case ValueType::TYPE_INT:
		{
			int value = std::stoi(str);
			SetValue2(value);
			break;
		}
		case ValueType::TYPE_FLOAT:
		{
			float value = std::stof(str);
			SetValue2(value);
			break;
		}
		default:
			return false;
		}
	}
	catch (const std::invalid_argument&)
	{
		return false;
	}
	catch (const std::out_of_range&)
	{
		return false;
	}

	return true;
}

bool WeaponEntityProperty::TestBoolean(CBaseEntity* player, CBaseEntity* weapon) const
{
	bool value = false;
	
	if (!entprops->GetEntPropBool(IsOnPlayer() ? player : weapon, GetPropType(), GetPropertyName().c_str(), value))
	{
		// property doesn't exists in the entity
		return false;
	}

	switch (m_compareType)
	{
	case CompareType::COMPARE_EQUALS:
	{
		return value == std::any_cast<bool>(m_value1);
	}
	case CompareType::COMPARE_NOTEQUALS:
	{
		return value != std::any_cast<bool>(m_value1);
	}
	default:
		break;
	}

	return false;
}

bool WeaponEntityProperty::TestInt(CBaseEntity* player, CBaseEntity* weapon) const
{
	int value = 0;

	if (!entprops->GetEntProp(IsOnPlayer() ? player : weapon, GetPropType(), GetPropertyName().c_str(), value))
	{
		// property doesn't exists in the entity
		return false;
	}

	switch (m_compareType)
	{
	case CompareType::COMPARE_EQUALS:
	{
		return value == std::any_cast<int>(m_value1);
	}
	case CompareType::COMPARE_NOTEQUALS:
	{
		return value != std::any_cast<int>(m_value1);
	}
	case CompareType::COMPARE_GREATER:
	{
		return value > std::any_cast<int>(m_value1);
	}
	case CompareType::COMPARE_LESS:
	{
		return value < std::any_cast<int>(m_value1);
	}
	case CompareType::COMPARE_BETWEEN:
	{
		return value > std::any_cast<int>(m_value1) && value < std::any_cast<int>(m_value2);
	}
	case CompareType::COMPARE_BIT_SET:
	{
		return (value & std::any_cast<int>(m_value1)) != 0;
	}
	default:
		break;
	}

	return false;
}

bool WeaponEntityProperty::TestFloat(CBaseEntity* player, CBaseEntity* weapon) const
{
	float value = 0;

	if (!entprops->GetEntPropFloat(IsOnPlayer() ? player : weapon, GetPropType(), GetPropertyName().c_str(), value))
	{
		// property doesn't exists in the entity
		return false;
	}

	switch (m_compareType)
	{
	case CompareType::COMPARE_GREATER:
	{
		return value > std::any_cast<float>(m_value1);
	}
	case CompareType::COMPARE_LESS:
	{
		return value < std::any_cast<float>(m_value1);
	}
	case CompareType::COMPARE_BETWEEN:
	{
		return value > std::any_cast<float>(m_value1) && value < std::any_cast<float>(m_value2);
	}
	default:
		break;
	}

	return false;
}

WeaponEntityProperty::ValueType WeaponEntityProperty::GetValueFromString(const std::string& str)
{
	if (str == "bool")
	{
		return ValueType::TYPE_BOOL;
	}

	if (str == "int")
	{
		return ValueType::TYPE_INT;
	}

	if (str == "float")
	{
		return ValueType::TYPE_FLOAT;
	}

	return ValueType::TYPE_UNKNOWN;
}

WeaponEntityProperty::CompareType WeaponEntityProperty::GetCompareTypeFromString(const std::string& str)
{
	if (str == "equals")
	{
		return CompareType::COMPARE_EQUALS;
	}

	if (str == "notequals")
	{
		return CompareType::COMPARE_NOTEQUALS;
	}

	if (str == "greater")
	{
		return CompareType::COMPARE_GREATER;
	}

	if (str == "less")
	{
		return CompareType::COMPARE_LESS;
	}

	if (str == "between")
	{
		return CompareType::COMPARE_BETWEEN;
	}

	if (str == "bitset")
	{
		return CompareType::COMPARE_BIT_SET;
	}

	return CompareType::COMPARE_NONE;
}


float WeaponAttackFunctionInfo::GetDistanceMappedAttackDelay(const float dist) const
{
	return RemapValClamped(dist, dmad.range_min, dmad.range_max, dmad.delay_min, dmad.delay_max);
}

bool WeaponAttackFunctionInfo::ParseDistanceMappedAttackDelay(const char* input)
{
	if (std::strcmp(input, "disable") == 0)
	{
		dmad.enabled = false;
		return true;
	}

	if (sscanf(input, "%f %f %f %f", &dmad.range_min, &dmad.range_max, &dmad.delay_min, &dmad.delay_max) == 4)
	{
		if (dmad.range_min >= dmad.range_max)
		{
			return false;
		}

		dmad.enabled = true;
		return true;
	}

	return false;
}

bool CWeaponInfoManager::LoadConfigFile()
{
	std::string spath;
	const CBaseMod* mod = extmanager->GetMod();

	if (!mod->BuildPathToModFile("configs/navbot", "weapons.cfg", "weapons.custom.cfg", spath))
	{
		smutils->LogError(myself, "Failed to load Weapon Info configuration file. File does not exists!");
		return false;
	}
	
	ClearData();
	InitParserData();
	SourceMod::SMCStates state;
	auto errorcode = textparsers->ParseFile_SMC(spath.c_str(), this, &state);

	if (errorcode != SourceMod::SMCError::SMCError_Okay)
	{
		smutils->LogError(myself, "Failed to parse Weapon Info configuration file \"%s\". Parser received error %i (%s)", 
			spath.c_str(), static_cast<int>(errorcode), textparsers->GetSMCErrorString(errorcode));

		return false;
	}

	PostParseAnalysis();
	RemoveTemplateEntries();
	BuildLookupMaps();

	auto functor = [](CBaseBot* bot) {
		bot->GetInventoryInterface()->OnWeaponInfoConfigReloaded();
	};

	extmanager->ForEachBot(functor);

#ifdef EXT_DEBUG
	META_CONPRINTF("[NavBot] Parsed weapon config file: %s \n", spath.c_str());
#endif // EXT_DEBUG

	return true;
}

bool CWeaponInfoManager::ParseCustomFile(const std::string& file)
{
	if (!std::filesystem::exists(file))
	{
		return false;
	}

	ClearData();
	InitParserData();
	SourceMod::SMCStates state;
	auto errorcode = textparsers->ParseFile_SMC(file.c_str(), this, &state);

	if (errorcode != SourceMod::SMCError::SMCError_Okay)
	{
		smutils->LogError(myself, "Failed to parse Weapon Info configuration file \"%s\". Parser received error %i (%s)",
			file.c_str(), static_cast<int>(errorcode), textparsers->GetSMCErrorString(errorcode));

		return false;
	}

	PostParseAnalysis();
	RemoveTemplateEntries();
	BuildLookupMaps();

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

		if (weaponinfoptr->IsTemplateEntry())
		{
			continue; // ignore all checks below for templates
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

void CWeaponInfoManager::ReadDynamicPrioritySection(const SourceMod::SMCStates* states, const char* key, const char* value)
{
	if (ke::StrCaseCmp(key, "erase") == 0)
	{
		m_current->EraseDynamicPrioritiesCfgEntry();
		return;
	}

	m_current->AddDynamicPriorityCfgEntry(key, value);
}

void CWeaponInfoManager::ReadAttackInfoSection(botweapons::AttackType type, const SourceMod::SMCStates* states, const char* key, const char* value)
{
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
		m_current->GetAttackInfoForEditing(type)->SetMelee(UtilHelpers::StringToBoolean(value));
	}
	else if (strncmp(key, "explosive", 7) == 0)
	{
		m_current->GetAttackInfoForEditing(type)->SetExplosive(UtilHelpers::StringToBoolean(value));
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
	else if (strncmp(key, "delay_between_attacks", 21) == 0)
	{
		float v = atof(value);

		if (v <= 0.0f)
		{
			v = -1.0f;
		}

		m_current->GetAttackInfoForEditing(type)->SetDelayBetweenAttacks(v);
	}
	else if (strncmp(key, "block_attacks_time", 18) == 0)
	{
		float v = atof(value);

		if (v <= 0.0f)
		{
			v = -1.0f;
		}

		m_current->GetAttackInfoForEditing(type)->SetBlockAttackTime(v);
	}
	else if (strncmp(key, "range_mapped_attack_delay", 25) == 0)
	{
		if (!m_current->GetAttackInfoForEditing(type)->ParseDistanceMappedAttackDelay(value))
		{
			smutils->LogError(myself, "[WEAPON INFO PARSER] Error parsing \"range_mapped_attack_delay\" (Attack Info %i) <%s - %s> at line %i col %i",
				static_cast<int>(type), key, value, states->line, states->col);
		}
	}
	else
	{
		smutils->LogError(myself, "[WEAPON INFO PARSER] Unknown key value pair (Attack Info %i) <%s - %s> at line %i col %i",
			static_cast<int>(type), key, value, states->line, states->col);
	}
}

void CWeaponInfoManager::BuildLookupMaps()
{
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
			auto insert_result = m_classname_lookup.insert({ std::string{ info->GetClassname() }, info.get() });

			if (!insert_result.second)
			{
				smutils->LogError(myself, "WeaponInfo: Failed to insert classname %s to look up map! Entry: %s", info->GetClassname(), info->GetConfigEntryName());
			}
		}
	}
}

const WeaponInfo* CWeaponInfoManager::GetWeaponInfo(const std::string& classname, const int index) const
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

SourceMod::SMCResult CWeaponInfoManager::ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name)
{
	/*
	
	WeaponInfoConfig // depth 0 (ROOT)
	{
		"entry" // depth 1 (WEAPON LIST)
		{
			"k1" "v1"
			...
			"sub_section" // depth 2 (WEAPON DATA)
			{
				...
			}
		}
	}
	
	*/

	if (m_parser_depth == PARSER_DEPTH_ROOT)
	{
		if (std::strcmp(name, "WeaponInfoConfig") == 0)
		{
			m_isvalid = true;
		}
		else
		{
			smutils->LogError(myself, "[WEAPON PARSER INFO] Invalid file!");
			return SourceMod::SMCResult_HaltFail;
		}
	}
	else if (m_parser_depth == PARSER_DEPTH_WEAPONLIST)
	{
		if (!m_section_weapon)
		{
			m_section_weapon = true;
			auto& newinfo = m_weapons.emplace_back(CreateWeaponInfo());
			m_current = newinfo.get();
			m_current->SetConfigEntryName(name);
		}
	}
	else if (m_parser_depth == PARSER_DEPTH_WEAPONDATA)
	{
		if (std::strcmp(name, "primary_attack_info") == 0)
		{
			m_section_prim = true;
		}
		else if (std::strcmp(name, "secondary_attack_info") == 0)
		{
			m_section_sec = true;
		}
		else if (std::strcmp(name, "tertiary_attack_info") == 0)
		{
			m_section_ter = true;
		}
		else if (std::strcmp(name, "special_function") == 0)
		{
			m_section_specialfunc = true;
		}
		else if (std::strcmp(name, "dynamic_priorities") == 0)
		{
			m_section_dynamicprio = true;
		}
		else
		{
			smutils->LogError(myself, "[WEAPON INFO CONFIG] Unknown weapon subsection named \"%s\"!", name);
		}
	}

	m_parser_depth++;
	return SMCResult_Continue;
}

SMCResult CWeaponInfoManager::ReadSMC_KeyValue(const SMCStates* states, const char* key, const char* value)
{
	if (m_section_specialfunc)
	{
		WeaponInfo::SpecialFunction* func = m_current->GetSpecialFunctionEx();

		if (std::strcmp(key, "setup_property") == 0)
		{
			if (!func->entprop.SetupFromString(value))
			{
				smutils->LogError(myself, "Failed to setup weapon special function entity prop! Line %u col %u", states->line, states->col);
			}
		}
		else if (std::strcmp(key, "button_to_press") == 0)
		{
			func->button_to_press = IPlayerInput::GetButtonIDFromString(value);
		}
		else if (std::strcmp(key, "delay_between_uses") == 0)
		{
			func->delay_between_uses = atof(value);
		}
		else if (std::strcmp(key, "hold_button_time") == 0)
		{
			func->hold_button_time = atof(value);
		}
		else if (std::strcmp(key, "min_range_to_activate") == 0)
		{
			func->min_range = atof(value);
		}
		else if (std::strcmp(key, "max_range_to_activate") == 0)
		{
			func->max_range = atof(value);
		}
		else
		{
			smutils->LogError(myself, "[WEAPON INFO PARSER] Unknown key value pair on special function <%s - %s> at line %i col %i", key, value, states->line, states->col);
		}

		return SourceMod::SMCResult::SMCResult_Continue;
	}

	if (m_section_dynamicprio)
	{
		ReadDynamicPrioritySection(states, key, value);
		return SourceMod::SMCResult::SMCResult_Continue;
	}

	if (std::strcmp(key, "variantof") == 0)
	{
		const WeaponInfo* other = this->LookUpWeaponInfoByConfigEntryName(value);

		if (!other)
		{
			smutils->LogError(myself, "[WEAPON INFO PARSER] Unknown config entry name \"%s\" used in \"variantof\" at line %u col %u!", value, states->line, states->col);
		}
		else
		{
			m_current->VariantOf(other);
		}
	}
	else if (std::strcmp(key, "is_template") == 0)
	{
		m_current->SetIsTemplateEntry(UtilHelpers::StringToBoolean(value));
	}
	else if (strncmp(key, "classname", 9) == 0)
	{
		m_current->SetClassname(value);
	}
	else if (strncmp(key, "ammo_source_entity", 18) == 0)
	{
		m_current->SetAmmoSourceEntityClassname(value);
	}
	else if (strncmp(key, "itemindex", 9) == 0)
	{
		m_current->SetEconItemIndex(atoi(value));
	}
	else if (std::strcmp(key, "priority") == 0)
	{
		m_current->SetPriority(atoi(value));
	}
	else if (std::strcmp(key, "min_required_skill") == 0)
	{
		int v = atoi(value);
		v = std::clamp(v, 0, 100);
		m_current->SetMinRequiredSkill(v);
	}
	else if (std::strcmp(key, "weapon_type") == 0)
	{
		WeaponInfo::WeaponType type = WeaponInfo::GetWeaponTypeFromString(value);

		if (type == WeaponInfo::WeaponType::MAX_WEAPON_TYPES)
		{
			smutils->LogError(myself, "[WEAPON INFO PARSER] %s is not a valid weapon type! line %u col %u", value, states->line, states->col);
		}
		else
		{
			m_current->SetWeaponType(type);
		}
	}
	else if (std::strcmp(key, "can_fire_underwater") == 0)
	{
		bool v = UtilHelpers::StringToBoolean(value);
		m_current->SetCanBeFiredUnderwater(v);
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
	else if (std::strcmp(key, "initial_attack_delay") == 0)
	{
		float v = atof(value);
		v = std::clamp(v, -1.0f, 30.0f);
		m_current->SetInitialAttackDelay(v);
	}
	else if (std::strcmp(key, "scopein_attack_delay") == 0)
	{
		float v = atof(value);
		v = std::clamp(v, -1.0f, 30.0f);
		m_current->SetScopeInAttackDelay(v);
	}
	else if (strncmp(key, "headshot_range_multiplier", 25) == 0)
	{
		float hsrange = atof(value);
		hsrange = std::clamp(hsrange, 0.0f, 1.0f);
		m_current->SetHeadShotRangeMultiplier(hsrange);
	}
	else if (std::strcmp(key, "spam_time") == 0)
	{
		float t = atof(value);
		t = std::clamp(t, -1.0f, 30.0f);
		m_current->SetSpamTime(t);
	}
	else if (std::strcmp(key, "scopein_min_range") == 0)
	{
		float range = atof(value);
		
		if (range <= 0.0f)
		{
			range = -1.0f;
		}

		m_current->SetMinimumRangeToUseScope(range);
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
		m_current->SetNoClip1(UtilHelpers::StringToBoolean(value));
	}
	else if (std::strcmp(key, "secondary_no_clip") == 0)
	{
		m_current->SetNoClip2(UtilHelpers::StringToBoolean(value));
	}
	else if (std::strcmp(key, "secondary_uses_primary_ammo_type") == 0)
	{
		m_current->SetSecondaryUsesPrimaryAmmoType(UtilHelpers::StringToBoolean(value));
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
		smutils->LogError(myself, "Deprecated key \"attack_interval\" at line %u col %u!", states->line, states->col);
	}
	else if (strncmp(key, "attack_range_override", 21) == 0)
	{
		float flvalue = atof(value);
		m_current->SetAttackRange(flvalue);
	}
	else if (strncmp(key, "use_secondary_attack_chance", 27) == 0)
	{
		int v = atoi(value);
		v = std::clamp(v, 0, 100);
		m_current->SetChanceToUseSecondaryAttack(v);
	}
	else if (std::strcmp(key, "setup_custom_ammo_property") == 0)
	{
		if (!m_current->SetupCustomAmmoProperty(value))
		{
			smutils->LogError(myself, "Error while parsing \"setup_custom_ammo_property\" at line %u col %u!", states->line, states->col);
		}
	}
	else if (std::strcmp(key, "is_deployed_property_setup") == 0)
	{
		if (!m_current->ParseDeployedProperty(value))
		{
			smutils->LogError(myself, "Error while parsing \"is_deployed_property_setup\" at line %u col %u!", states->line, states->col);
		}
	}
	else if (std::strcmp(key, "needs_to_be_deployed_to_fire") == 0)
	{
		bool v = UtilHelpers::StringToBoolean(value);
		m_current->SetNeedsToBeDeployed(v);
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
	else if (std::strcmp(key, "tags") == 0)
	{
		m_current->ClearTags();

		std::string szValue(value);
		std::stringstream stream(szValue);
		std::string token;

		while (std::getline(stream, token, ','))
		{
			if (m_current->HasTag(token))
			{
				smutils->LogError(myself, "[WEAPON INFO PARSER] Duplicate tag \"%s\" found on \"%s\" at line %u col %u!",
					token.c_str(), m_current->GetConfigEntryName(), states->line, states->col);
			}

			m_current->AddTag(token);
		}
	}
	else if (std::strcmp(key, "add_tags") == 0)
	{
		// add new tags while keeping the current ones (for when variantof is used)

		std::string szValue(value);
		std::stringstream stream(szValue);
		std::string token;

		while (std::getline(stream, token, ','))
		{
			if (m_current->HasTag(token))
			{
				smutils->LogError(myself, "[WEAPON INFO PARSER] Duplicate tag \"%s\" found on \"%s\" at line %u col %u!",
					token.c_str(), m_current->GetConfigEntryName(), states->line, states->col);
			}

			m_current->AddTag(token);
		}
	}
	else if (std::strcmp(key, "clear_tags") == 0)
	{
		m_current->ClearTags();
	}
	else if (std::strcmp(key, "remove_tags") == 0)
	{
		std::string szValue(value);
		std::stringstream stream(szValue);
		std::string token;

		while (std::getline(stream, token, ','))
		{
			m_current->RemoveTag(token);
		}
	}
	else if (std::strcmp(key, "preferred_aim_spot") == 0)
	{
		if (std::strcmp(value, "head") == 0)
		{
			m_current->SetPreferredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_HEAD);
		}
		else if (std::strcmp(value, "center") == 0)
		{
			m_current->SetPreferredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER);
		}
		else if (std::strcmp(value, "origin") == 0 || std::strcmp(value, "feet") == 0)
		{
			m_current->SetPreferredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_ABSORIGIN);
		}
		else
		{
			smutils->LogError(myself, "[WEAPON INFO PARSER] Invalid value for \"preferred_aim_spot\" at line %u col %u: \"%s\"", states->line, states->col, value);
		}
	}
	else if (!IsParserInWeaponAttackSection())
	{
		smutils->LogError(myself, "[WEAPON INFO PARSER] Unknown key value pair <%s - %s> at line %u col %u", key, value, states->line, states->col);
	}

	if (IsParserInWeaponAttackSection())
	{
		botweapons::AttackType type = botweapons::AttackType::PRIMARY;

		if (m_section_sec)
		{
			type = botweapons::AttackType::SECONDARY;
		}
		else if (m_section_ter)
		{
			type = botweapons::AttackType::TERTIARY;
		}

		ReadAttackInfoSection(type, states, key, value);
	}

	return SourceMod::SMCResult::SMCResult_Continue;
}

SourceMod::SMCResult CWeaponInfoManager::ReadSMC_LeavingSection(const SourceMod::SMCStates* states)
{
	/*
	WeaponInfoConfig
	{
		// leaves at depth 1
		"entry"
		{
			// leaves at depth 2
			"k1" "v1"
			...
			"sub_section"
			{
				// leaves at depth 3
				...
			}
		}
	}
	*/


	if (m_parser_depth == 3)
	{
		if (m_section_specialfunc)
		{
			m_section_specialfunc = false;
		}
		else if (m_section_dynamicprio)
		{
			m_section_dynamicprio = false;
		}
		else if (IsParserInWeaponAttackSection())
		{
			ParserExitWeaponSection();
		}
	}
	else if (m_parser_depth == 2)
	{
		if (m_section_weapon)
		{
			m_section_weapon = false;
			m_current = nullptr;
		}
	}

	m_parser_depth--;
	return SourceMod::SMCResult_Continue;
}

WeaponInfo::WeaponType WeaponInfo::GetWeaponTypeFromString(const char* str)
{
	using namespace std::literals::string_view_literals;

	constexpr std::array type_names = {
		"not_useable_weapon"sv,
		"combat_weapon"sv,
		"buff_item"sv,
		"defensive_buff_item"sv,
		"heal_item"sv,
		"medical_weapon"sv,
		"mobility_tool"sv,
		"combat_grenade"sv,
		"support_grenade"sv,
	};

	static_assert(type_names.size() == static_cast<std::size_t>(WeaponInfo::WeaponType::MAX_WEAPON_TYPES), "WeaponInfo::WeaponType and type_names array size mismatch!");

	for (int i = 0; i < static_cast<int>(WeaponInfo::WeaponType::MAX_WEAPON_TYPES); i++)
	{
		const std::string_view& name = type_names[i];

		if (std::strcmp(str, name.data()) == 0)
		{
			return static_cast<WeaponInfo::WeaponType>(i);
		}
	}

	return WeaponInfo::WeaponType::MAX_WEAPON_TYPES;
}

void WeaponInfo::AddDynamicPriorityCfgEntry(const char* name, const char* args)
{
	auto& entry = parsed_dynamic_prio_configs.emplace_back();
	entry.name.assign(name);

	std::size_t count = 0U;
	std::string szValue(args);
	std::stringstream stream(szValue);
	std::string token;

	while (std::getline(stream, token, ','))
	{
		entry.args.emplace_back(token);
	}
}

void WeaponInfo::PostLoad()
{
	// If not set by the config file, use the largest distance between all available attacks.
	if (attack_move_range <= 0.0f)
	{
		for (size_t i = 0; i < static_cast<size_t>(botweapons::AttackType::MAX_ATTACK_TYPES); i++)
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
		float minrange = std::numeric_limits<float>::max();

		for (size_t i = 0; i < static_cast<size_t>(botweapons::AttackType::MAX_ATTACK_TYPES); i++)
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

	// setup dynamic priorities
	CDynamicPriorityManager& manager = CDynamicPriorityManager::GetManager();


	for (const DynamicPrioCfg& entry : parsed_dynamic_prio_configs)
	{
		const CDynamicPriorityManager::Factory* factory = manager.FindFactory(entry.name);

		if (!factory)
		{
			smutils->LogError(myself, "WeaponInfo entry \"%s\", NULL dynamic priority factory for \"%s\"!", configentry.c_str(), entry.name.c_str());
			continue;
		}

		IDynamicWeaponPriority* priority = factory->Create();
		
		if (!priority->Configure(entry.args))
		{
			delete priority;
			smutils->LogError(myself, "WeaponInfo entry \"%s\", configuration of dynamic priority factory \"%s\" failed!", configentry.c_str(), entry.name.c_str());
			continue;
		}

		dynamic_priorities.emplace_back(priority);
	}

	// don't need this anymore
	parsed_dynamic_prio_configs.clear();
	parsed_dynamic_prio_configs.shrink_to_fit();
}

CON_COMMAND(sm_navbot_reload_weaponinfo_config, "Reloads the weapon info configuration file.")
{
	extmanager->GetMod()->ReloadWeaponInfoConfigFile();
}
