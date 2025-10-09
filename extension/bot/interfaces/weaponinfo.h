#ifndef NAVBOT_WEAPON_INFO_H_
#define NAVBOT_WEAPON_INFO_H_
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <cmath>

#include <ITextParsers.h>
#include <bot/interfaces/decisionquery.h>

class WeaponAttackFunctionInfo
{
public:
	WeaponAttackFunctionInfo()
	{
		maxrange = -10000.0f;
		minrange = -1.0f;
		projectilespeed = -1.0f;
		gravity = -1.0f;
		ballistic_elevation_range_start = -1.0f;
		ballistic_elevation_range_end = -1.0f;
		ballistic_elevation_min = -1.0f;
		ballistic_elevation_max = -1.0f;
		hold_time = -1.0f;
		ismelee = false;
		isexplosive = false;
	}

	void Reset()
	{
		maxrange = -10000.0f;
		minrange = -1.0f;
		projectilespeed = -1.0f;
		gravity = -1.0f;
		ballistic_elevation_range_start = -1.0f;
		ballistic_elevation_range_end = -1.0f;
		ballistic_elevation_min = -1.0f;
		ballistic_elevation_max = -1.0f;
		hold_time = -1.0f;
		ismelee = false;
		isexplosive = false;
	}

	float GetMaxRange() const { return maxrange; }
	float GetMinRange() const { return minrange; }
	float GetProjectileSpeed() const { return projectilespeed; }
	float GetGravity() const { return gravity; }
	float GetBallisticElevationStartRange() const { return ballistic_elevation_range_start; }
	float GetBallisticElevationEndRange() const { return ballistic_elevation_range_end; }
	float GetBallisticElevationMinRate() const { return ballistic_elevation_min; }
	float GetBallisticElevationMaxRate() const { return ballistic_elevation_max; }
	bool IsMelee() const { return ismelee; }
	bool IsExplosive() const { return isexplosive; }
	bool IsHitscan() const { return projectilespeed <= 0.0f; }
	// fires a projectile?
	bool IsProjectile() const { return projectilespeed > 0.0f; }
	// fires a projectile that is affected by gravity?
	bool IsBallistic() const { return gravity > 0.0f; }
	bool HasMaxRange() const { return maxrange > 0.0f; }
	bool HasMinRange() const { return minrange > 0.0f; }
	bool InRangeTo(const float dist) const { return dist >= minrange && dist <= maxrange; }
	bool InRangeToSqr(const float dist) const { return dist >= (minrange * minrange) && dist <= (maxrange * maxrange); }
	float GetTravelTimeForDistance(const float dist) const { return dist / projectilespeed; }
	bool HasFunction() const { return maxrange > -9000.0f; }
	float GetHoldButtonTime() const { return hold_time; }

	void SetMaxRange(float v) { maxrange = v; }
	void SetMinRange(float v) { minrange = v; }
	void SetProjectileSpeed(float v) { projectilespeed = v; }
	void SetGravity(float v) { gravity = v; }
	void SetBallisticElevationStartRange(float v) { ballistic_elevation_range_start = v; }
	void SetBallisticElevationEndRange(float v) { ballistic_elevation_range_end = v; }
	void SetBallisticElevationMin(float v) { ballistic_elevation_min = v; }
	void SetBallisticElevationMax(float v) { ballistic_elevation_max = v; }
	void SetMelee(bool v) { ismelee = v; }
	void SetExplosive(bool v) { isexplosive = v; }
	void SetHoldButtonTime(float v) { hold_time = v; }

private:
	float maxrange;
	float minrange;
	float projectilespeed;
	float gravity;
	float ballistic_elevation_range_start;
	float ballistic_elevation_range_end;
	float ballistic_elevation_min;
	float ballistic_elevation_max;
	float hold_time;
	bool ismelee;
	bool isexplosive;
};

class WeaponInfo
{
public:
	// Random Number used to indicate an invalid weapon slot
	static constexpr int INVALID_WEAPON_SLOT = -7912742;

	enum AttackFunctionType
	{
		PRIMARY_ATTACK = 0,
		SECONDARY_ATTACK,
		TERTIARY_ATTACK,

		MAX_WEAPON_ATTACKS
	};

	enum WeaponType
	{
		NOT_USEABLE_WEAPON, // Can't be used for anything
		COMBAT_WEAPON, // (Default) Weapon used for combat.
		BUFF_ITEM, // Provides a buff (IE: TF2 Crit-a-Cola, Soldier's banners)
		DEFENSIVE_BUFF_ITEM, // Privdes a defensive buff (IE: TF2 bonk)
		HEAL_ITEM, // Can be used to self heal
		MEDICAL_WEAPON, // Can heal teammates (IE: TF2 medigun)
		MOBILITY_TOOL, // Weapon provides extra mobility (grappling hook, jetpack, ...)
		COMBAT_GRENADE, // Standard grenade for combat (IE: explosive grenades)
		SUPPORT_GRENADE, // Support grenades (smoke, flashes, ...)

		MAX_WEAPON_TYPES
	};

	// Converts a string to a weapon type
	static WeaponInfo::WeaponType GetWeaponTypeFromString(const char* str);

	struct SpecialFunction
	{
		SpecialFunction()
		{
			property_on_weapon = true;
			property_is_float = true;
			available_threshold = -1.0f;
			hold_button_time = -1.0f;
			delay_between_uses = -1.0f;
			min_range = -1.0f;
			max_range = 900000.0f;
			button_to_press = 0;
		}

		std::string property_name;
		bool property_on_weapon;
		bool property_is_float;
		float available_threshold;
		float hold_button_time;
		float delay_between_uses;
		float min_range;
		float max_range;
		int button_to_press;
	};

	struct DynamicPriority
	{
		DynamicPriority()
		{
			is_used = false;
			is_greater = false;
			value_to_compare = 0.0f;
			priority_value = 0;
		}

		void Parse(const char* str);

		bool is_used;
		bool is_greater; // if true, compare is greater, otherwise less
		float value_to_compare;
		int priority_value;
	};

	WeaponInfo() :
		headshot_aim_offset(0.0f, 0.0f, 0.0f)
	{
		classname.reserve(64);
		configentry.reserve(64);
		weapon_type = WeaponType::COMBAT_WEAPON;
		custom_ammo_property_name.reserve(64);
		econindex = -1;
		priority = 0;
		disable_dodge = false;
		can_headshot = false;
		infinite_reserve_ammo = false;
		custom_ammo_prop_on_weapon = true;
		custom_ammo_prop_is_net = true;
		custom_ammo_is_float = false;
		custom_ammo_out_of_ammo = 0.0f;
		interval_between_attacks = -1.0f;
		initial_attack_delay = -1.0f;
		scopein_attack_delay = -1.0f;
		headshot_range_mult = 1.0f;
		spam_time = -1.0f;
		scopein_min_range = -1.0f;
		no_clip1 = false;
		no_clip2 = false;
		sec_uses_prim = false;
		is_template = false;
		primammolow = 0;
		secammolow = 0;
		slot = INVALID_WEAPON_SLOT;
		attack_move_range = -1.0f;
		use_secondary_chance = 20;
		deployed_property_name.reserve(64);
		needs_to_be_deployed = false;
		deployed_property_on_weapon = true;
		selection_max_range = -1.0f;
		selection_min_range = -1.0f;
		min_required_skill = 0;
		preferred_aim_spot = IDecisionQuery::DesiredAimSpot::AIMSPOT_NONE;
	}

	virtual ~WeaponInfo() {}

	// This WI instance is a variant of the given WI, copy the 'other' attributes into this one.
	virtual void VariantOf(const WeaponInfo* other)
	{
		this->classname = other->classname;
		this->custom_ammo_property_name = other->custom_ammo_property_name;
		this->attacksinfo = other->attacksinfo;
		this->weapon_type = other->weapon_type;
		this->headshot_aim_offset = other->headshot_aim_offset;
		this->econindex = other->econindex;
		this->priority = other->priority;
		this->disable_dodge = other->disable_dodge;
		this->can_headshot = other->can_headshot;
		this->infinite_reserve_ammo = other->infinite_reserve_ammo;
		this->custom_ammo_prop_on_weapon = other->custom_ammo_prop_on_weapon;
		this->custom_ammo_prop_is_net = other->custom_ammo_prop_is_net;
		this->custom_ammo_is_float = other->custom_ammo_is_float;
		this->custom_ammo_out_of_ammo = other->custom_ammo_out_of_ammo;
		this->interval_between_attacks = other->interval_between_attacks;
		this->initial_attack_delay = other->initial_attack_delay;
		this->scopein_attack_delay = other->scopein_attack_delay;
		this->headshot_range_mult = other->headshot_range_mult;
		this->spam_time = other->spam_time;
		this->scopein_min_range = other->scopein_min_range;
		this->no_clip1 = other->no_clip1;
		this->no_clip2 = other->no_clip2;
		this->sec_uses_prim = other->sec_uses_prim;
		this->is_template = false; // copies are never a template
		this->primammolow = other->primammolow;
		this->secammolow = other->secammolow;
		this->slot = other->slot;
		this->attack_move_range = other->attack_move_range;
		this->use_secondary_chance = other->use_secondary_chance;
		this->deployed_property_name = other->deployed_property_name;
		this->deployed_property_on_weapon = other->deployed_property_on_weapon;
		this->needs_to_be_deployed = other->needs_to_be_deployed;
		this->selection_max_range = other->selection_max_range;
		this->selection_min_range = other->selection_min_range;
		this->special_function = other->special_function;
		this->tags = other->tags;
		this->dynprio_health = other->dynprio_health;
		this->dynprio_range = other->dynprio_range;
		this->dynprio_sec_ammo = other->dynprio_sec_ammo;
		this->dynprio_aggression = other->dynprio_aggression;
		this->min_required_skill = other->min_required_skill;
		this->preferred_aim_spot = other->preferred_aim_spot;
	}

	const WeaponAttackFunctionInfo& operator[](AttackFunctionType type) const
	{
		return attacksinfo[type];
	}

	const char* GetClassname() const { return classname.c_str(); }
	const std::string& GetClassnameString() const { return classname; }
	const char* GetConfigEntryName() const { return configentry.c_str(); }
	const std::string& GetConfigEntryNameString() const { return configentry; }
	int GetPriority() const { return priority; }
	int GetItemDefIndex() const { return econindex; }

	void SetAttackInfo(AttackFunctionType type, WeaponAttackFunctionInfo info)
	{
		attacksinfo[type] = info;
	}

	void SetClassname(const char* szclassname)
	{
		classname.assign(szclassname);
	}

	void SetConfigEntryName(const char* entry)
	{
		configentry.assign(entry);
	}

	void SetCanHeadShot(bool v) { can_headshot = v; }
	void SetInfiniteReserveAmmo(bool v) { infinite_reserve_ammo = v; }
	void SetHeadShotRangeMultiplier(float v) { headshot_range_mult = v; }
	void SetHeadShotAimOffset(const Vector& offset) { headshot_aim_offset = offset; }

	WeaponAttackFunctionInfo* GetAttackInfoForEditing(AttackFunctionType type)
	{
		return &attacksinfo[type];
	}

	const WeaponAttackFunctionInfo& GetAttackInfo(AttackFunctionType type) const
	{
		return attacksinfo[type];
	}

	// Returns true if this is the default weapon info profile
	bool IsDefault() const { return configentry.size() == 0; }
	bool CanHeadShot() const { return can_headshot; }
	bool HasInfiniteReserveAmmo() const { return infinite_reserve_ammo; }
	float GetHeadShotRangeMultiplier() const { return headshot_range_mult; }
	float GetMaxPrimaryHeadShotRange() const { return attacksinfo[PRIMARY_ATTACK].GetMaxRange() * headshot_range_mult; }
	const Vector& GetHeadShotAimOffset() const { return headshot_aim_offset; }

	void SetEconItemIndex(int index) { econindex = index; }
	void SetPriority(int pri) { priority = pri; }
	void SetNoClip1(bool v) { no_clip1 = v; }
	void SetNoClip2(bool v) { no_clip2 = v; }
	void SetSecondaryUsesPrimaryAmmoType(bool v) { sec_uses_prim = v; }
	void SetLowPrimaryAmmoThreshold(int v) { primammolow = v; }
	void SetLowSecondaryAmmoThreshold(int v) { secammolow = v; }
	void SetSlot(int s) { slot = s; }
	void SetAttackInterval(float v) { interval_between_attacks = v; }
	void SetInitialAttackDelay(float v) { initial_attack_delay = v; }
	void SetScopeInAttackDelay(float v) { scopein_attack_delay = v; }
	void SetAttackRange(float v) { attack_move_range = v; }
	void SetChanceToUseSecondaryAttack(int v) { use_secondary_chance = v; }
	void SetCustomAmmoPropertyName(const char* name) { custom_ammo_property_name.assign(name); }
	void SetCustomAmmoPropertySource(bool onweapon) { custom_ammo_prop_on_weapon = onweapon; }
	void SetCustomAmmoPropertyType(bool networked) { custom_ammo_prop_is_net = networked; }
	void SetCustomAmmoOutOfAmmoThreshold(float v) { custom_ammo_out_of_ammo = v; }
	void SetCustomAmmoPropertyIsFloat(const bool v) { custom_ammo_is_float = v; }
	void SetDeployedPropertyName(const char* name) { deployed_property_name.assign(name); }
	void SetNeedsToBeDeployed(const bool v) { needs_to_be_deployed = v; }
	void SetDeployedPropertySource(const bool onweapon) { deployed_property_on_weapon = onweapon; }
	void SetIsDodgeDisabled(const bool v) { disable_dodge = v; }
	void SetSelectionMaxRangeOverride(const float v) { selection_max_range = v; }
	void ClearTags() { tags.clear(); }
	void AddTag(const std::string& tag) { tags.emplace(tag); }
	void RemoveTag(const std::string& tag) { tags.erase(tag); }
	void SetIsTemplateEntry(const bool v) { is_template = v; }
	void SetWeaponType(WeaponInfo::WeaponType type) { weapon_type = type; }
	DynamicPriority* EditDynamicPriorityHealth() { return &dynprio_health; }
	DynamicPriority* EditDynamicPriorityRange() { return &dynprio_range; }
	DynamicPriority* EditDynamicPrioritySecAmmo() { return &dynprio_sec_ammo; }
	DynamicPriority* EditDynamicPriorityAggression() { return &dynprio_aggression; }
	void SetMinRequiredSkill(const int skill) { min_required_skill = skill; }
	void SetSpamTime(const float t) { spam_time = t; }
	void SetMinimumRangeToUseScope(const float range) { scopein_min_range = range; }
	void SetPreferredAimSpot(IDecisionQuery::DesiredAimSpot spot) { preferred_aim_spot = spot; }

	bool HasEconIndex() const { return econindex >= 0; }
	bool IsEntry(std::string& entry) const { return configentry == entry; }
	bool IsClassname(std::string& name) const { return classname == name; }
	bool HasPrimaryAttack() const { return attacksinfo[PRIMARY_ATTACK].HasFunction(); }
	bool HasSecondaryAttack() const { return attacksinfo[SECONDARY_ATTACK].HasFunction(); }
	bool HasTertiaryAttack() const { return attacksinfo[TERTIARY_ATTACK].HasFunction(); }
	// Returns true for weapons that can be used in combat
	bool IsCombatWeapon() const
	{
		switch (weapon_type)
		{
		case WeaponType::COMBAT_WEAPON:
			[[fallthrough]];
		case WeaponType::COMBAT_GRENADE:
			[[fallthrough]];
		case WeaponType::BUFF_ITEM:
			[[fallthrough]];
		case WeaponType::DEFENSIVE_BUFF_ITEM:
			[[fallthrough]];
		case WeaponType::MOBILITY_TOOL:
			return true;
		default:
			return false;
		}
	}
	bool HasLowPrimaryAmmoThreshold() const { return primammolow > 0; }
	int GetLowPrimaryAmmoThreshold() const { return primammolow; }
	bool HasLowSecondaryAmmoThreshold() const { return secammolow > 0; }
	int GetLowSecondaryAmmoThreshold() const { return secammolow; }
	int GetSlot() const { return slot; }
	bool HasSlot() const { return slot != INVALID_WEAPON_SLOT; }
	float GetAttackInterval() const { return interval_between_attacks; }
	float GetInitialAttackDelay() const { return initial_attack_delay; }
	float GetScopeInAttackDelay() const { return scopein_attack_delay; }
	// if this returns true, the weapon doesn't uses clips for the primary attack
	bool Clip1IsReserveAmmo() const { return no_clip1; }
	// if this returns true, the weapon doesn't uses clips for the secondary attack
	bool Clip2IsReserveAmmo() const { return no_clip1; }
	// Returns true if the weapon secondary attack uses the primary ammo type.
	bool SecondaryUsesPrimaryAmmo() const { return sec_uses_prim; }
	// Returns the minimum distance bots should try to maintain when attacking
	const float GetAttackRange() const { return attack_move_range; }
	int GetChanceToUseSecondaryAttack() const { return use_secondary_chance; }
	const std::string& GetCustomAmmoPropertyName() const { return custom_ammo_property_name; }
	bool HasCustomAmmoProperty() const { return !custom_ammo_property_name.empty(); }
	bool IsCustomAmmoPropertyOnWeapon() const { return custom_ammo_prop_on_weapon; }
	bool IsCustomAmmoPropertyNetworked() const { return custom_ammo_prop_is_net; }
	float GetCustomAmmoOutOfAmmoThreshold() const { return custom_ammo_out_of_ammo; }
	bool IsCustomAmmoPropertyAFloat() const { return custom_ammo_is_float;  }
	bool HasDeployedStateProperty() const { return !deployed_property_name.empty(); }
	const std::string& GetDeployedPropertyName() const { return deployed_property_name; }
	bool NeedsToBeDeployedToFire() const { return needs_to_be_deployed; }
	bool IsDeployedPropertyOnTheWeapon() const { return deployed_property_on_weapon; }
	bool IsAllowedToDodge() const { return !disable_dodge; }
	bool IsInSelectionRange(const float range) const
	{
		if (range >= selection_min_range && range <= selection_max_range)
		{
			return true;
		}

		return false;
	}
	bool HasTag(const std::string& tag) const { return tags.find(tag) != tags.end(); }
	bool IsTemplateEntry() const { return is_template; }
	WeaponInfo::WeaponType GetWeaponType() const { return weapon_type; }
	const bool IsWeaponOfType(const WeaponInfo::WeaponType type) const { return type == weapon_type; }
	const SpecialFunction& GetSpecialFunction() const { return special_function; }
	// for writing
	SpecialFunction* GetSpecialFunctionEx() { return &special_function; }
	const DynamicPriority& GetDynamicPriorityHealth() const { return dynprio_health; }
	const DynamicPriority& GetDynamicPriorityRange() const { return dynprio_range; }
	const DynamicPriority& GetDynamicPrioritySecAmmo() const { return dynprio_sec_ammo; }
	const DynamicPriority& GetDynamicPriorityAggression() const { return dynprio_aggression; }
	const int GetMinRequiredSkill() const { return min_required_skill; }
	const float GetSpamTime() const { return spam_time; }
	const bool CanBeSpammed() const { return spam_time > 0.0f; }
	const float GetMinimumScopeInRange() const { return scopein_min_range; }
	IDecisionQuery::DesiredAimSpot GetPreferredAimSpot() const { return preferred_aim_spot; }
	const bool HasAimSpotPreference() const { return preferred_aim_spot != IDecisionQuery::DesiredAimSpot::AIMSPOT_NONE; }
	// Returns true if the bot is allowed to use the weapon's scope in the given range between the bot and the enemy.
	const bool IsAllowedToScopeIn(const float range) const
	{
		if (scopein_min_range > 0.0f)
		{
			return range >= scopein_min_range;
		}

		return true;
	}
 
	virtual void PostLoad();

private:
	std::string classname;
	std::string configentry;
	std::string custom_ammo_property_name;
	std::array<WeaponAttackFunctionInfo, AttackFunctionType::MAX_WEAPON_ATTACKS> attacksinfo;
	WeaponType weapon_type;
	Vector headshot_aim_offset;
	int econindex; // Economy item definition index
	int priority; // Priority for weapon selection
	bool disable_dodge; // if true, the bot won't try to dodge enemy attacks while using this weapon
	bool can_headshot;
	bool infinite_reserve_ammo; // weapon has infinite reserve ammo (no need to collect ammo for it)
	bool custom_ammo_prop_on_weapon; // if true, the custom ammo property is located on the weapon, if false, on the player
	bool custom_ammo_prop_is_net; // if true, the custom ammo property is a networked property, else it's a datamap
	bool custom_ammo_is_float; // if true, the custom ammo property is a float, else an integer.
	float custom_ammo_out_of_ammo; // if the custom ammo is equal or less than this, the weapon is out of ammo
	float interval_between_attacks; // delay between attacks
	float initial_attack_delay; // delay before the bot should start attacking after entering combat.
	float scopein_attack_delay; // delay before the bot should atart attacking after scoping in with the weapon.
	float headshot_range_mult;
	float spam_time; // how long to keep firing after losing LOS
	float scopein_min_range;
	bool no_clip1; // Weapon doesn't uses clip 1 (primary)
	bool no_clip2; // Weapon doesn't uses clip 2 (secondary)
	bool sec_uses_prim; // Secondary attack uses primary ammo type
	bool is_template; // This entry is a template for "variantof" (disables some error checks)
	int primammolow; // Threshold for low primary ammo
	int secammolow; // Threshold for low secondary ammo
	int slot; // Slot used by this weapon. Used when selecting a weapon by slot.
	float attack_move_range; // Minimum distance the bot will try to maintain when attacking.
	int use_secondary_chance; // Chance to use the secondary attack if available
	std::string deployed_property_name;
	bool deployed_property_on_weapon; // if true, the property is on the weapon, else is on the player.
	bool needs_to_be_deployed; // if true, the weapon needs to be deployed/scoped to fire it.
	float selection_max_range;
	float selection_min_range;
	int min_required_skill;
	SpecialFunction special_function;
	std::unordered_set<std::string> tags;
	DynamicPriority dynprio_health;
	DynamicPriority dynprio_range;
	DynamicPriority dynprio_sec_ammo;
	DynamicPriority dynprio_aggression;
	IDecisionQuery::DesiredAimSpot preferred_aim_spot;
};

class CWeaponInfoManager : public SourceMod::ITextListener_SMC
{
public:
	CWeaponInfoManager()
	{
		m_weapons.reserve(128);
		m_isvalid = false;
		m_section_weapon = false;
		m_section_prim = false;
		m_section_sec = false;
		m_section_ter = false;
		m_section_specialfunc = false;
		m_section_dynamicprio = false;
		m_parser_depth = 0;
		m_current = nullptr;
	}

	virtual ~CWeaponInfoManager() {}

protected:
	virtual WeaponInfo* CreateWeaponInfo() const { return new WeaponInfo; }
public:

	bool WeaponEntryExists(std::string& entry) const
	{
		for (auto& weaponptr : m_weapons)
		{
			if (weaponptr->IsEntry(entry))
			{
				return true;
			}
		}

		return false;
	}


	/**
	 * @brief Gets a weapon info by classname or econ index. Econ index has priority
	 * @param classname Classname to search
	 * @param index Econ index to search
	 * @return A weaponinfo is always returned, if not found, a default is returned.
	 */
	virtual const WeaponInfo* GetWeaponInfo(const std::string& classname, const int index) const;

	bool IsWeaponInfoLoaded() const { return m_weapons.size() > 0; }

protected:
	/**
	 * @brief Called when entering a new section
	 *
	 * @param states		Parsing states.
	 * @param name			Name of section, with the colon omitted.
	 * @return				SMCResult directive.
	 */
	SMCResult ReadSMC_NewSection(const SMCStates* states, const char* name) override;

	/**
	 * @brief Called when encountering a key/value pair in a section.
	 *
	 * @param states		Parsing states.
	 * @param key			Key string.
	 * @param value			Value string.  If no quotes were specified, this will be NULL,
	 *						and key will contain the entire string.
	 * @return				SMCResult directive.
	 */
	SMCResult ReadSMC_KeyValue(const SMCStates* states, const char* key, const char* value) override;

	/**
	 * @brief Called when leaving the current section.
	 *
	 * @param states		Parsing states.
	 * @return				SMCResult directive.
	 */
	SMCResult ReadSMC_LeavingSection(const SMCStates* states) override;

public:

	bool LoadConfigFile();

protected:
	std::vector<std::unique_ptr<WeaponInfo>> m_weapons; // main storage
	std::unique_ptr<WeaponInfo> m_default; // Default weapon info for when lookup fails
	std::unordered_map<int, const WeaponInfo*> m_index_lookup; // map for econ index look up
	std::unordered_map<std::string, const WeaponInfo*> m_classname_lookup; // map for classname look up

	void InitParserData()
	{
		m_isvalid = false;
		m_section_weapon = false;
		m_section_prim = false;
		m_section_sec = false;
		m_section_ter = false;
		m_section_specialfunc = false;
		m_section_dynamicprio = false;
		m_parser_depth = 0;
		m_current = nullptr;
	}

	// parser data
	bool m_isvalid;
	bool m_section_weapon;
	bool m_section_prim; // primary attack section
	bool m_section_sec; // secondary attack section
	bool m_section_ter; // tertiary attack section
	bool m_section_specialfunc; // special function section
	bool m_section_dynamicprio; // dynamic priority section
	int m_parser_depth; // section depth

	WeaponInfo* m_current; // Current weapon info being parsed

	bool IsParserInWeaponAttackSection() const
	{
		return m_section_prim || m_section_sec || m_section_ter;
	}

	void ParserExitWeaponSection()
	{
		// only one section is parsed at a time so this should be safe
		m_section_prim = false;
		m_section_sec = false;
		m_section_ter = false;
	}

	const WeaponInfo* LookUpWeaponInfoByClassname(const std::string& classname) const
	{
		auto it = m_classname_lookup.find(classname);

		if (it != m_classname_lookup.end())
		{
			return it->second;
		}

		return nullptr;
	}

	const WeaponInfo* LookUpWeaponInfoByEconIndex(const int index) const
	{
		if (index < 0) { return nullptr; }

		auto it = m_index_lookup.find(index);

		if (it != m_index_lookup.end())
		{
			return it->second;
		}

		return nullptr;
	}

	// Searches for a weapon info by config entry name, used by the parser
	const WeaponInfo* LookUpWeaponInfoByConfigEntryName(const char* name) const
	{
		for (auto& info : m_weapons)
		{
			if (std::strcmp(info->GetConfigEntryName(), name) == 0)
			{
				return info.get();
			}
		}

		return nullptr;
	}

	virtual void PostParseAnalysis();

private:

	void ReadDynamicPrioritySection(const SourceMod::SMCStates* states, const char* key, const char* value);
};

#endif // !NAVBOT_WEAPON_INFO_H_

