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
#include <any>

#include <ITextParsers.h>
#include <util/entprops_consts.h>
#include <bot/interfaces/decisionquery.h>
#include <bot/interfaces/playerinput.h>
#include "weapons_shared.h"

/**
 * @brief Stores information about entity properties for weapon configs.
 */
class WeaponEntityProperty
{
public:
	// Value types supported
	enum ValueType
	{
		TYPE_UNKNOWN = 0,
		TYPE_BOOL,
		TYPE_INT,
		TYPE_FLOAT,

		MAX_SUPPORTED_TYPES
	};

	static ValueType GetValueFromString(const std::string& str);

	enum CompareType
	{
		COMPARE_NONE = 0,
		COMPARE_EQUALS, // property value must be equals
		COMPARE_NOTEQUALS, // property value must not equals
		COMPARE_GREATER, // property value must be greater
		COMPARE_LESS, // property value must be less
		COMPARE_BETWEEN, // property value must be between value1 and value2
		COMPARE_BIT_SET, // check if the bit (value1) is set on property value

		MAX_SUPPORTED_COMPARE_TYPES
	};

	static CompareType GetCompareTypeFromString(const std::string& str);

	WeaponEntityProperty()
	{
		m_propName.reserve(64);
		m_propType = Prop_Send;
		m_valueType = TYPE_UNKNOWN;
		m_compareType = COMPARE_NONE;
		m_onPlayer = false;
	}

	WeaponEntityProperty& operator=(const WeaponEntityProperty& other)
	{
		if (this == &other) { return *this; }

		m_propName = other.m_propName;
		m_propType = other.m_propType;
		m_compareType = other.m_compareType;
		m_onPlayer = other.m_onPlayer;
		m_value1 = other.m_value1;
		m_value2 = other.m_value2;
		return *this;
	}

	void SetPropertyName(const char* name) { m_propName.assign(name); }
	void SetPropType(PropType type) { m_propType = type; }
	void SetValueType(ValueType type) { m_valueType = type; }
	void SetCompareType(CompareType type) { m_compareType = type; }
	void SetOnPlayer(bool state) { m_onPlayer = state; }
	template <typename T>
	void SetValue1(T value) { m_value1 = std::make_any<T>(value); }
	template <typename T>
	void SetValue2(T value) { m_value2 = std::make_any<T>(value); }
	const std::string& GetPropertyName() const { return m_propName; }
	PropType GetPropType() const { return m_propType; }
	ValueType GetValueType() const { return m_valueType; }
	CompareType GetCompareType() const { return m_compareType; }
	bool IsOnPlayer() const { return m_onPlayer; }

	/**
	 * @brief Setups the property from a string. Call this when parsing config files.
	 * @param str Input string.
	 * @return True if the configuration is correct, false otherwise.
	 */
	bool SetupFromString(const char* str);

	bool IsValid() const
	{
		if (m_valueType == ValueType::TYPE_UNKNOWN) { return false; }
		if (m_compareType == CompareType::COMPARE_NONE) { return false; }
		if (!m_value1.has_value()) { return false; }
		// only allow equality tests for boleans
		if (m_valueType == ValueType::TYPE_BOOL && 
			!(m_compareType == CompareType::COMPARE_EQUALS || m_compareType == CompareType::COMPARE_NOTEQUALS)) { return false; }
		// don't allow equality tests for floats
		if (m_valueType == ValueType::TYPE_FLOAT &&
			(m_compareType == CompareType::COMPARE_EQUALS || m_compareType == CompareType::COMPARE_NOTEQUALS)) {
			return false;
		}
		// only allow bit tests with ints
		if (m_compareType == CompareType::COMPARE_BIT_SET && m_valueType != ValueType::TYPE_INT) { return false; }
		// these compare types requires the value 2 to be set.
		if ((m_compareType == CompareType::COMPARE_BETWEEN || m_compareType == CompareType::COMPARE_BIT_SET) && 
			!m_value2.has_value()) { return false; }

		return true;
	}

	bool HasPropertyName() const { return !m_propName.empty(); }
	/**
	 * @brief Tests the property condition.
	 * @param player Player who owns the weapon.
	 * @param weapon The weapon entity.
	 * @return True if the condition matches, false otherwise.
	 */
	bool TestCondition(CBaseEntity* player, CBaseEntity* weapon) const;
	/**
	 * @brief Reads the property.
	 * @param player Player who owns the weapon.
	 * @param weapon Weapon entity.
	 * @return Property value as boolean.
	 */
	int ReadPropBool(CBaseEntity* player, CBaseEntity* weapon) const;
	/**
	 * @brief Reads the property.
	 * @param player Player who owns the weapon.
	 * @param weapon Weapon entity.
	 * @return Property value as int.
	 */
	int ReadPropInt(CBaseEntity* player, CBaseEntity* weapon) const;
	/**
	 * @brief Reads the property.
	 * @param player Player who owns the weapon.
	 * @param weapon Weapon entity.
	 * @return Property value as float.
	 */
	float ReadPropFloat(CBaseEntity* player, CBaseEntity* weapon) const;

private:
	std::string m_propName;
	PropType m_propType;
	ValueType m_valueType;
	CompareType m_compareType;
	bool m_onPlayer; // true if the property is on the player, false if on the weapon's entity
	std::any m_value1; // first value
	std::any m_value2; // second value

	bool ReadValue1(const std::string& str);
	bool ReadValue2(const std::string& str);
	void Reset()
	{
		m_propName.clear();
		m_propType = Prop_Send;
		m_valueType = ValueType::TYPE_UNKNOWN;
		m_compareType = CompareType::COMPARE_NONE;
		m_onPlayer = false;
		m_value1.reset();
		m_value2.reset();
	}
	bool TestBoolean(CBaseEntity* player, CBaseEntity* weapon) const;
	bool TestInt(CBaseEntity* player, CBaseEntity* weapon) const;
	bool TestFloat(CBaseEntity* player, CBaseEntity* weapon) const;
};

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
		delay_between_attacks = -1.0f;
		block_attacks_time = -1.0f;
		ismelee = false;
		isexplosive = false;
		dmad.enabled = false;
		dmad.range_max = 0.0f;
		dmad.range_max = 0.0f;
		dmad.delay_min = 0.0f;
		dmad.delay_max = 0.0f;
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
	float GetDelayBetweenAttacks() const { return delay_between_attacks; }
	float GetBlockAttackTime() const { return block_attacks_time; }
	bool UsesDistanceMappedAttackDelays() const { return dmad.enabled; }
	float GetDistanceMappedAttackDelay(const float dist) const;

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
	void SetDelayBetweenAttacks(float v) { delay_between_attacks = v; }
	void SetBlockAttackTime(float time) { block_attacks_time = time; }
	bool ParseDistanceMappedAttackDelay(const char* input);

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
	float delay_between_attacks;
	float block_attacks_time;
	bool ismelee;
	bool isexplosive;

	struct DistanceMappedAttackDelay
	{
		bool enabled;
		float range_min;
		float range_max;
		float delay_min;
		float delay_max;
	};

	DistanceMappedAttackDelay dmad;
};

class WeaponInfo
{
public:
	// Random Number used to indicate an invalid weapon slot
	static constexpr int INVALID_WEAPON_SLOT = -7912742;

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
			hold_button_time = -1.0f;
			delay_between_uses = -1.0f;
			min_range = -1.0f;
			max_range = 900000.0f;
			button_to_press = IPlayerInput::ButtonID::BUTTON_INVALID;
		}

		// Returns true if a special function is enabled
		bool IsEnabled() const { return entprop.HasPropertyName() && button_to_press != IPlayerInput::ButtonID::BUTTON_INVALID; }

		float hold_button_time;
		float delay_between_uses;
		float min_range;
		float max_range;
		IPlayerInput::ButtonID button_to_press;
		WeaponEntityProperty entprop;
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
		econindex = -1;
		priority = 0;
		disable_dodge = false;
		can_headshot = false;
		infinite_reserve_ammo = false;
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
		needs_to_be_deployed = false;
		can_fire_underwater = true;
		selection_max_range = -1.0f;
		selection_min_range = -1.0f;
		min_required_skill = 0;
		preferred_aim_spot = IDecisionQuery::DesiredAimSpot::AIMSPOT_NONE;
		ammo_source_classname.reserve(32);
	}

	virtual ~WeaponInfo() {}

	// This WI instance is a variant of the given WI, copy the 'other' attributes into this one.
	virtual void VariantOf(const WeaponInfo* other)
	{
		this->classname = other->classname;
		this->attacksinfo = other->attacksinfo;
		this->weapon_type = other->weapon_type;
		this->headshot_aim_offset = other->headshot_aim_offset;
		this->econindex = other->econindex;
		this->priority = other->priority;
		this->disable_dodge = other->disable_dodge;
		this->can_headshot = other->can_headshot;
		this->infinite_reserve_ammo = other->infinite_reserve_ammo;
		this->custom_ammo_prop = other->custom_ammo_prop;
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
		this->is_deployed_prop = other->is_deployed_prop;
		this->can_fire_underwater = other->can_fire_underwater;
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
		this->ammo_source_classname = other->ammo_source_classname;
	}

	const WeaponAttackFunctionInfo& operator[](botweapons::AttackType type) const
	{
		return attacksinfo[static_cast<int>(botweapons::AttackType::PRIMARY)];
	}

	const char* GetClassname() const { return classname.c_str(); }
	const std::string& GetClassnameString() const { return classname; }
	const char* GetConfigEntryName() const { return configentry.c_str(); }
	const std::string& GetConfigEntryNameString() const { return configentry; }
	int GetPriority() const { return priority; }
	int GetItemDefIndex() const { return econindex; }

	void SetAttackInfo(botweapons::AttackType type, WeaponAttackFunctionInfo info)
	{
		attacksinfo[static_cast<int>(type)] = info;
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

	WeaponAttackFunctionInfo* GetAttackInfoForEditing(botweapons::AttackType type)
	{
		return &attacksinfo[static_cast<int>(type)];
	}

	const WeaponAttackFunctionInfo& GetAttackInfo(botweapons::AttackType type) const
	{
		return attacksinfo[static_cast<int>(type)];
	}

	// Returns true if this is the default weapon info profile
	bool IsDefault() const { return configentry.size() == 0; }
	bool CanHeadShot() const { return can_headshot; }
	bool HasInfiniteReserveAmmo() const { return infinite_reserve_ammo; }
	float GetHeadShotRangeMultiplier() const { return headshot_range_mult; }
	float GetMaxPrimaryHeadShotRange() const { return attacksinfo[static_cast<int>(botweapons::AttackType::PRIMARY)].GetMaxRange() * headshot_range_mult; }
	const Vector& GetHeadShotAimOffset() const { return headshot_aim_offset; }

	void SetEconItemIndex(int index) { econindex = index; }
	void SetPriority(int pri) { priority = pri; }
	void SetNoClip1(bool v) { no_clip1 = v; }
	void SetNoClip2(bool v) { no_clip2 = v; }
	void SetSecondaryUsesPrimaryAmmoType(bool v) { sec_uses_prim = v; }
	void SetLowPrimaryAmmoThreshold(int v) { primammolow = v; }
	void SetLowSecondaryAmmoThreshold(int v) { secammolow = v; }
	void SetSlot(int s) { slot = s; }
	void SetInitialAttackDelay(float v) { initial_attack_delay = v; }
	void SetScopeInAttackDelay(float v) { scopein_attack_delay = v; }
	void SetAttackRange(float v) { attack_move_range = v; }
	void SetChanceToUseSecondaryAttack(int v) { use_secondary_chance = v; }
	bool SetupCustomAmmoProperty(const char* value) { return custom_ammo_prop.SetupFromString(value); }
	bool ParseDeployedProperty(const char* value) { return is_deployed_prop.SetupFromString(value); }
	void SetNeedsToBeDeployed(const bool v) { needs_to_be_deployed = v; }
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
	void SetAmmoSourceEntityClassname(const char* classname) { ammo_source_classname.assign(classname); }
	void SetCanBeFiredUnderwater(bool state) { can_fire_underwater = state; }

	bool HasEconIndex() const { return econindex >= 0; }
	bool IsEntry(std::string& entry) const { return configentry == entry; }
	bool IsClassname(std::string& name) const { return classname == name; }
	bool HasPrimaryAttack() const { return attacksinfo[static_cast<int>(botweapons::AttackType::PRIMARY)].HasFunction(); }
	bool HasSecondaryAttack() const { return attacksinfo[static_cast<int>(botweapons::AttackType::SECONDARY)].HasFunction(); }
	bool HasTertiaryAttack() const { return attacksinfo[static_cast<int>(botweapons::AttackType::TERTIARY)].HasFunction(); }
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
	bool HasCustomAmmoProperty() const { return custom_ammo_prop.HasPropertyName(); }
	const WeaponEntityProperty& GetCustomAmmoProperty() const { return custom_ammo_prop; }
	bool HasDeployedStateProperty() const { return is_deployed_prop.HasPropertyName(); }
	const WeaponEntityProperty& GetDeployedProperty() const { return is_deployed_prop; }
	bool NeedsToBeDeployedToFire() const { return needs_to_be_deployed; }
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
	const bool HasAmmoSourceEntityClassname() const { return !ammo_source_classname.empty(); }
	const std::string& GetAmmoSourceEntityClassname() const { return ammo_source_classname; }
	const bool CanBeFiredUnderwater() const { return can_fire_underwater; }

	virtual void PostLoad();

private:
	std::string classname;
	std::string configentry;
	std::array<WeaponAttackFunctionInfo, static_cast<std::size_t>(botweapons::AttackType::MAX_ATTACK_TYPES)> attacksinfo;
	WeaponType weapon_type;
	Vector headshot_aim_offset;
	int econindex; // Economy item definition index
	int priority; // Priority for weapon selection
	bool disable_dodge; // if true, the bot won't try to dodge enemy attacks while using this weapon
	bool can_headshot;
	bool infinite_reserve_ammo; // weapon has infinite reserve ammo (no need to collect ammo for it)
	WeaponEntityProperty custom_ammo_prop; // custom ammo property.
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
	WeaponEntityProperty is_deployed_prop; // Property used to determine if the weapon is deployed/scoped.
	bool needs_to_be_deployed; // if true, the weapon needs to be deployed/scoped to fire it.
	bool can_fire_underwater; // if true, the weapon can be fired while underwater
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
	std::string ammo_source_classname;
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

	/**
	 * @brief Gets a weapon info via classname.
	 * @param classname Classname to search.
	 * @return Weapon info if found or NULL.
	 */
	const WeaponInfo* GetClassnameInfo(const std::string& classname) const
	{
		return LookUpWeaponInfoByClassname(classname);
	}

	bool IsWeaponInfoLoaded() const { return m_weapons.size() > 0; }

protected:
	/**
	 * @brief Called when entering a new section
	 *
	 * @param states		Parsing states.
	 * @param name			Name of section, with the colon omitted.
	 * @return				SMCResult directive.
	 */
	SourceMod::SMCResult ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name) override;

	/**
	 * @brief Called when encountering a key/value pair in a section.
	 *
	 * @param states		Parsing states.
	 * @param key			Key string.
	 * @param value			Value string.  If no quotes were specified, this will be NULL,
	 *						and key will contain the entire string.
	 * @return				SMCResult directive.
	 */
	SourceMod::SMCResult ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value) override;

	/**
	 * @brief Called when leaving the current section.
	 *
	 * @param states		Parsing states.
	 * @return				SMCResult directive.
	 */
	SourceMod::SMCResult ReadSMC_LeavingSection(const SourceMod::SMCStates* states) override;

public:

	bool LoadConfigFile();
	// Parses a custom weapon config file. Used by the SourcePawn API.
	bool ParseCustomFile(const std::string& file);

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

	static constexpr int PARSER_DEPTH_ROOT = 0; // root section
	static constexpr int PARSER_DEPTH_WEAPONLIST = 1; // weapons list
	static constexpr int PARSER_DEPTH_WEAPONDATA = 2; // weapon data

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
	void ReadAttackInfoSection(botweapons::AttackType type, const SourceMod::SMCStates* states, const char* key, const char* value);
	void ClearData()
	{
		m_index_lookup.clear();
		m_classname_lookup.clear();
		m_weapons.clear();
	}
	void RemoveTemplateEntries()
	{
		// purge templates before inserting into the look up database
		m_weapons.erase(std::remove_if(m_weapons.begin(), m_weapons.end(), [](const std::unique_ptr<WeaponInfo>& obj) {
			return obj->IsTemplateEntry();
		}), m_weapons.end());
	}
	void BuildLookupMaps();
};

#endif // !NAVBOT_WEAPON_INFO_H_

