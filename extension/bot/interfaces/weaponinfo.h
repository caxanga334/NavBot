#ifndef NAVBOT_WEAPON_INFO_H_
#define NAVBOT_WEAPON_INFO_H_
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <optional>
#include <cmath>

#include <ITextParsers.h>

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

	WeaponInfo() :
		headshot_aim_offset(0.0f, 0.0f, 0.0f)
	{
		classname.reserve(64);
		configentry.reserve(64);
		econindex = -1;
		priority = 0;
		can_headshot = false;
		infinite_reserve_ammo = false;
		interval_between_attacks = -1.0f;
		headshot_range_mult = 1.0f;
		maxclip1 = 0;
		maxclip2 = 0;
		primammolow = 0;
		secammolow = 0;
		slot = INVALID_WEAPON_SLOT;
		attack_move_range = -1.0f;
		use_secondary_chance = 20;
	}

	virtual ~WeaponInfo() {}

	void Reset()
	{
		classname.clear();
		configentry.clear();
		econindex = -1;
		priority = 0;
		can_headshot = false;
		infinite_reserve_ammo = false;
		interval_between_attacks = -1.0f;
		headshot_range_mult = 1.0f;
		headshot_aim_offset.Init(0.0f, 0.0f, 0.0f);
		maxclip1 = 0;
		maxclip2 = 0;
		primammolow = 0;
		secammolow = 0;
		attack_move_range = -1.0f;
		use_secondary_chance = 20;

		slot = INVALID_WEAPON_SLOT;
		attacksinfo[PRIMARY_ATTACK].Reset();
		attacksinfo[SECONDARY_ATTACK].Reset();
		attacksinfo[TERTIARY_ATTACK].Reset();
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
	void SetMaxClip1(int clip) { maxclip1 = clip; }
	void SetMaxClip2(int clip) { maxclip2 = clip; }
	void SetLowPrimaryAmmoThreshold(int v) { primammolow = v; }
	void SetLowSecondaryAmmoThreshold(int v) { secammolow = v; }
	void SetSlot(int s) { slot = s; }
	void SetAttackInterval(float v) { interval_between_attacks = v; }
	void SetAttackRange(float v) { attack_move_range = v; }
	void SetChanceToUseSecondaryAttack(int v) { use_secondary_chance = v; }

	bool HasEconIndex() const { return econindex >= 0; }
	bool IsEntry(std::string& entry) const { return configentry == entry; }
	bool IsClassname(std::string& name) const { return classname == name; }
	bool HasPrimaryAttack() const { return attacksinfo[PRIMARY_ATTACK].HasFunction(); }
	bool HasSecondaryAttack() const { return attacksinfo[SECONDARY_ATTACK].HasFunction(); }
	bool HasTertiaryAttack() const { return attacksinfo[TERTIARY_ATTACK].HasFunction(); }
	bool IsCombatWeapon() const { return priority >= 0; }
	bool HasMaxClip1() const { return maxclip1 > 0; }
	int GetMaxClip1() const { return maxclip1; }
	bool HasMaxClip2() const { return maxclip2 > 0; }
	int GetMaxClip2() const { return maxclip2; }
	bool HasLowPrimaryAmmoThreshold() const { return primammolow > 0; }
	int GetLowPrimaryAmmoThreshold() const { return primammolow; }
	bool HasLowSecondaryAmmoThreshold() const { return secammolow > 0; }
	int GetLowSecondaryAmmoThreshold() const { return secammolow; }
	int GetSlot() const { return slot; }
	bool HasSlot() const { return slot != INVALID_WEAPON_SLOT; }
	float GetAttackInterval() const { return interval_between_attacks; }
	bool Clip1IsReserveAmmo() const { return maxclip1 == CLIP_USES_RESERVE; }
	bool Clip2IsReserveAmmo() const { return maxclip2 == CLIP_USES_RESERVE; }
	// Returns true if the weapon secondary attack uses the primary ammo type.
	bool SecondaryUsesPrimaryAmmo() const { return maxclip2 == SECONDARY_ATTACK_USES_PRIMARY_AMMO; }
	// Returns the minimum distance bots should try to maintain when attacking
	const float GetAttackRange() const { return attack_move_range; }
	int GetChanceToUseSecondaryAttack() const { return use_secondary_chance; }
 
	virtual void PostLoad();

protected:
	std::string classname;
	std::string configentry;
	WeaponAttackFunctionInfo attacksinfo[MAX_WEAPON_ATTACKS];
	Vector headshot_aim_offset;
	int econindex; // Economy item definition index
	int priority; // Priority for weapon selection
	bool can_headshot;
	bool infinite_reserve_ammo; // weapon has infinite reserve ammo (no need to collect ammo for it)
	float interval_between_attacks; // delay between attacks
	float headshot_range_mult;
	int maxclip1; // Maximum ammo stored in clip1
	int maxclip2; // Maximum ammo stored in clip2
	int primammolow; // Threshold for low primary ammo
	int secammolow; // Threshold for low secondary ammo
	int slot; // Slot used by this weapon. Used when selecting a weapon by slot.
	float attack_move_range; // Minimum distance the bot will try to maintain when attacking.
	int use_secondary_chance; // Chance to use the secondary attack if available

	static constexpr auto CLIP_USES_RESERVE = -2; // if maxclip is equal to this constant, then the weapon doesn't use clips and the actual ammo in the 'clip' is the reserve ammo.
	static constexpr auto SECONDARY_ATTACK_USES_PRIMARY_AMMO = -3;
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
	virtual const WeaponInfo* GetWeaponInfo(std::string classname, const int index) const;

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
	}

	// parser data
	bool m_isvalid;
	bool m_section_weapon;
	bool m_section_prim; // primary attack section
	bool m_section_sec; // secondary attack section
	bool m_section_ter; // tertiary attack section

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

	const WeaponInfo* LookUpWeaponInfoByClassname(std::string classname) const
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

	virtual void PostParseAnalysis();
};

#endif // !NAVBOT_WEAPON_INFO_H_

