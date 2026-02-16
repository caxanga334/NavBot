#include NAVBOT_PCH_FILE
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
	std::string spath;
	const CBaseMod* mod = extmanager->GetMod();

	if (!mod->BuildPathToModFile("configs/navbot", "weapons.cfg", "weapons.custom.cfg", spath))
	{
		smutils->LogError(myself, "Failed to load Weapon Info configuration file. File does not exists!");
		return false;
	}

	m_index_lookup.clear();
	m_classname_lookup.clear();
	m_weapons.clear();

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

	// purge templates before inserting into the look up database
	m_weapons.erase(std::remove_if(m_weapons.begin(), m_weapons.end(), [](const std::unique_ptr<WeaponInfo>& obj) {
		return obj->IsTemplateEntry();
	}), m_weapons.end());

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

#ifdef EXT_DEBUG
	META_CONPRINTF("[NavBot] Parsed weapon config file: %s \n", spath.c_str());
#endif // EXT_DEBUG

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
	if (std::strcmp(key, "health") == 0)
	{
		m_current->EditDynamicPriorityHealth()->Parse(value);
	}
	else if (std::strcmp(key, "range") == 0)
	{
		m_current->EditDynamicPriorityRange()->Parse(value);
	}
	else if (std::strcmp(key, "secondary_ammo") == 0)
	{
		m_current->EditDynamicPrioritySecAmmo()->Parse(value);
	}
	else if (std::strcmp(key, "aggression") == 0)
	{
		m_current->EditDynamicPriorityAggression()->Parse(value);
	}
	else
	{
		smutils->LogError(myself, "Unknown KV pair in Dynamic Priority section! %s %s col %u line %u", key, value, states->col, states->line);
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

		if (std::strcmp(key, "property_name") == 0)
		{
			func->property_name.assign(value);
		}
		else if (std::strcmp(key, "property_source") == 0)
		{
			if (std::strcmp(value, "player") == 0)
			{
				func->property_on_weapon = false;
			}
			else if (std::strcmp(value, "weapon") == 0)
			{
				func->property_on_weapon = true;
			}
		}
		else if (std::strcmp(key, "property_is_float") == 0)
		{
			func->property_is_float = UtilHelpers::StringToBoolean(value);
		}
		else if (std::strcmp(key, "available_threshold") == 0)
		{
			func->available_threshold = atof(value);
		}
		else if (std::strcmp(key, "button_to_press") == 0)
		{
			if (std::strcmp(value, "secondary_attack") == 0)
			{
				func->button_to_press = INPUT_ATTACK2;
			}
			if (std::strcmp(value, "tertiary_attack") == 0)
			{
				func->button_to_press = INPUT_ATTACK3;
			}
			else if (std::strcmp(value, "reload") == 0)
			{
				func->button_to_press = INPUT_RELOAD;
			}
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

		return SourceMod::SMCResult_Continue;
	}

	if (m_section_dynamicprio)
	{
		ReadDynamicPrioritySection(states, key, value);
		return SourceMod::SMCResult_Continue;
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
		v = std::clamp(v, 0, 100);
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
		else
		{
			smutils->LogError(myself, "[WEAPON INFO PARSER] Unknown key value pair (Attack Info %i) <%s - %s> at line %i col %i", 
				static_cast<int>(type), key, value, states->line, states->col);
		}
	}

	return SMCResult_Continue;
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

	if (!special_function.property_name.empty())
	{
		if (special_function.button_to_press == 0)
		{
			smutils->LogError(myself, "Weapon Info for \"%s\" has special function declared with missing button to press.", this->configentry.c_str());
		}
	}
}

void WeaponInfo::DynamicPriority::Parse(const char* str)
{
	try
	{
		std::string szValue(str);
		std::stringstream stream(szValue);
		std::string token;
		std::uint8_t i = 0U;

		while (std::getline(stream, token, ','))
		{
			if (i == 0)
			{
				if (std::strcmp(token.c_str(), "remove") == 0)
				{
					this->is_used = false;
					this->is_greater = false;
					this->value_to_compare = 0.0f;
					this->priority_value = 0;
					return;
				}

				if (std::strcmp(token.c_str(), "empty") == 0)
				{
					i++;
					continue;
				}

				if (std::strcmp(token.c_str(), "greater") == 0)
				{
					this->is_greater = true;
				}
				else
				{
					this->is_greater = false;
				}
			}
			else if (i == 1)
			{
				if (std::strcmp(token.c_str(), "empty") == 0)
				{
					i++;
					continue;
				}

				this->value_to_compare = std::stof(token, nullptr);
			}
			else if (i == 2)
			{
				this->priority_value = std::stoi(token, nullptr);
			}

			if (++i >= 3)
			{
				break;
			}
		}

		this->is_used = true;
	}
	catch (std::exception& ex)
	{
		smutils->LogError(myself, "C++ Exception throw while parsing weapon dynamic priority value \"%s\" : %s", str, ex.what());
		this->is_used = false;
	}
}


CON_COMMAND(sm_navbot_reload_weaponinfo_config, "Reloads the weapon info configuration file.")
{
	extmanager->GetMod()->ReloadWeaponInfoConfigFile();
}