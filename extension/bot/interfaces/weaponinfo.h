#ifndef NAVBOT_WEAPON_INFO_H_
#define NAVBOT_WEAPON_INFO_H_
#pragma once

#include <string>
#include <vector>

#include <ITextParsers.h>

class WeaponAttackFunctionInfo
{
public:
	inline WeaponAttackFunctionInfo()
	{
		maxrange = -10000.0f;
		minrange = -1.0f;
		projectilespeed = -1.0f;
		gravity = -1.0f;
		ismelee = false;
		isexplosive = false;
	}

	inline WeaponAttackFunctionInfo(float max, float min = -1.0f, float spd = -1.0f, float grav = -1.0f, bool melee = false, bool exp = false)
	{
		maxrange = max;
		minrange = min;
		projectilespeed = spd;
		gravity = grav;
		ismelee = melee;
		isexplosive = exp;
	}

	inline WeaponAttackFunctionInfo& operator=(const WeaponAttackFunctionInfo& other)
	{
		this->maxrange = other.maxrange;
		this->minrange = other.minrange;
		this->projectilespeed = other.projectilespeed;
		this->gravity = other.gravity;
		this->ismelee = other.ismelee;
		this->isexplosive = other.isexplosive;
		return *this;
	}

	inline void Reset()
	{
		maxrange = -10000.0f;
		minrange = -1.0f;
		projectilespeed = -1.0f;
		gravity = -1.0f;
		ismelee = false;
		isexplosive = false;
	}

	inline float GetMaxRange() const { return maxrange; }
	inline float GetMinRange() const { return minrange; }
	inline float GetProjectileSpeed() const { return projectilespeed; }
	inline float GetGravity() const { return gravity; }
	inline bool IsMelee() const { return ismelee; }
	inline bool IsExplosive() const { return isexplosive; }
	inline bool IsHitscan() const { return projectilespeed <= 0.0f; }
	inline bool HasMaxRange() const { return maxrange > 0.0f; }
	inline bool HasMinRange() const { return minrange > 0.0f; }
	inline bool InRangeTo(const float dist) const { return dist >= minrange && dist <= maxrange; }
	inline bool InRangeToSqr(const float dist) const { return dist >= (minrange * minrange) && dist <= (maxrange * maxrange); }
	inline float GetTravelTimeForDistance(const float dist) const { return dist / projectilespeed; }
	inline bool HasFunction() const { return maxrange > -9000.0f; }

	inline void SetMaxRange(float v) { maxrange = v; }
	inline void SetMinRange(float v) { minrange = v; }
	inline void SetProjectileSpeed(float v) { projectilespeed = v; }
	inline void SetGravity(float v) { gravity = v; }
	inline void SetMelee(bool v) { ismelee = v; }
	inline void SetExplosive(bool v) { isexplosive = v; }

private:
	float maxrange;
	float minrange;
	float projectilespeed;
	float gravity;
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

	inline WeaponInfo()
	{
		classname.reserve(64);
		configentry.reserve(64);
		econindex = -1;
		priority = 0;
		can_headshot = false;
		headshot_range_mult = 1.0f;
		maxclip1 = 0;
		maxclip2 = 0;
		primammolow = 0;
		secammolow = 0;
		slot = INVALID_WEAPON_SLOT;
	}

	inline void Reset()
	{
		classname.clear();
		configentry.clear();
		econindex = -1;
		priority = 0;
		can_headshot = false;
		headshot_range_mult = 1.0f;
		maxclip1 = 0;
		maxclip2 = 0;
		primammolow = 0;
		secammolow = 0;

		slot = INVALID_WEAPON_SLOT;
		attacksinfo[PRIMARY_ATTACK].Reset();
		attacksinfo[SECONDARY_ATTACK].Reset();
		attacksinfo[TERTIARY_ATTACK].Reset();
	}

	inline const WeaponAttackFunctionInfo& operator[](AttackFunctionType type) const
	{
		return attacksinfo[type];
	}

	inline const char* GetClassname() const { return classname.c_str(); }
	inline const char* GetConfigEntryName() const { return configentry.c_str(); }
	inline int GetPriority() const { return priority; }
	inline int GetItemDefIndex() const { return econindex; }

	inline void SetAttackInfo(AttackFunctionType type, WeaponAttackFunctionInfo info)
	{
		attacksinfo[type] = info;
	}

	inline void SetClassname(const char* szclassname)
	{
		classname.assign(szclassname);
	}

	inline void SetConfigEntryName(const char* entry)
	{
		configentry.assign(entry);
	}

	inline void SetCanHeadShot(bool v) { can_headshot = v; }
	inline void SetHeadShotRangeMultiplier(float v) { headshot_range_mult = v; }

	WeaponAttackFunctionInfo* GetAttackInfoForEditing(AttackFunctionType type)
	{
		return &attacksinfo[type];
	}

	inline const WeaponAttackFunctionInfo& GetAttackInfo(AttackFunctionType type) const
	{
		return attacksinfo[type];
	}

	// Returns true if this is the default weapon info profile
	inline bool IsDefault() const { return configentry.size() == 0; }
	inline bool CanHeadShot() const { return can_headshot; }
	inline float GetHeadShotRangeMultiplier() const { return headshot_range_mult; }
	inline float GetMaxPrimaryHeadShotRange() const { return attacksinfo[PRIMARY_ATTACK].GetMaxRange() * headshot_range_mult; }

	inline void SetEconItemIndex(int index) { econindex = index; }
	inline void SetPriority(int pri) { priority = pri; }
	inline void SetMaxClip1(int clip) { maxclip1 = clip; }
	inline void SetMaxClip2(int clip) { maxclip2 = clip; }
	inline void SetLowPrimaryAmmoThreshold(int v) { primammolow = v; }
	inline void SetLowSecondaryAmmoThreshold(int v) { secammolow = v; }
	inline void SetSlot(int s) { slot = s; }

	inline bool HasEconIndex() const { return econindex >= 0; }
	inline bool IsEntry(std::string& entry) const { return configentry == entry; }
	inline bool IsClassname(std::string& name) const { return classname == name; }
	inline bool HasPrimaryAttack() const { return attacksinfo[PRIMARY_ATTACK].HasFunction(); }
	inline bool HasSecondaryAttack() const { return attacksinfo[SECONDARY_ATTACK].HasFunction(); }
	inline bool HasTertiaryAttack() const { return attacksinfo[TERTIARY_ATTACK].HasFunction(); }
	inline bool IsCombatWeapon() const { return priority >= 0; }
	inline bool HasMaxClip1() const { return maxclip1 > 0; }
	inline int GetMaxClip1() const { return maxclip1; }
	inline bool HasMaxClip2() const { return maxclip2 > 0; }
	inline int GetMaxClip2() const { return maxclip2; }
	inline bool HasLowPrimaryAmmoThreshold() const { return primammolow > 0; }
	inline int GetLowPrimaryAmmoThreshold() const { return primammolow; }
	inline bool HasLowSecondaryAmmoThreshold() const { return secammolow > 0; }
	inline int GetLowSecondaryAmmoThreshold() const { return secammolow; }
	inline int GetSlot() const { return slot; }
	inline bool HasSlot() const { return slot != INVALID_WEAPON_SLOT; }

private:
	std::string classname;
	std::string configentry;
	WeaponAttackFunctionInfo attacksinfo[MAX_WEAPON_ATTACKS];
	int econindex; // Economy item definition index
	int priority; // Priority for weapon selection
	bool can_headshot;
	float headshot_range_mult;
	int maxclip1; // Maximum ammo stored in clip1
	int maxclip2; // Maximum ammo stored in clip2
	int primammolow; // Threshold for low primary ammo
	int secammolow; // Threshold for low secondary ammo
	int slot; // Slot used by this weapon. Used when selecting a weapon by slot.
};

class CWeaponInfoManager : public SourceMod::ITextListener_SMC
{
public:
	inline CWeaponInfoManager() :
		m_tempweapinfo(),
		m_defaultinfo()
	{
		m_weapons.reserve(512);
		m_isvalid = false;
		m_section_weapon = false;
		m_section_prim = false;
		m_section_sec = false;
		m_section_ter = false;
	}

	inline bool WeaponEntryExists(std::string& entry) const
	{
		for (auto& weapon : m_weapons)
		{
			if (weapon.IsEntry(entry))
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
	inline const WeaponInfo& GetWeaponInfo(const char* classname, const int index) const
	{
		WeaponInfo result;

		if (LookUpWeaponInfoByEconIndex(index, result))
		{
			return result;
		}

		if (LookUpWeaponInfoByClassname(classname, result))
		{
			return result;
		}

		return m_defaultinfo;
	}

	inline bool IsWeaponInfoLoaded() const { return m_weapons.size() > 0; }

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

	bool LoadConfigFile();

private:
	std::vector<WeaponInfo> m_weapons;

	inline void InitParserData()
	{
		m_isvalid = false;
		m_section_weapon = false;
		m_section_prim = false;
		m_section_sec = false;
		m_section_ter = false;
		m_tempweapinfo.Reset();
	}

	// parser data
	bool m_isvalid;
	bool m_section_weapon;
	bool m_section_prim; // primary attack section
	bool m_section_sec; // secondary attack section
	bool m_section_ter; // tertiary attack section

	WeaponInfo m_tempweapinfo; // temporary weapon info
	WeaponInfo m_defaultinfo;

	inline bool IsParserInWeaponAttackSection() const
	{
		return m_section_prim || m_section_sec || m_section_ter;
	}

	inline void ParserExitWeaponSection()
	{
		// only one section is parsed at a time so this should be safe
		m_section_prim = false;
		m_section_sec = false;
		m_section_ter = false;
	}

	inline bool LookUpWeaponInfoByClassname(const char* classname, WeaponInfo& result) const
	{
		std::string name(classname);

		for (auto& weapon : m_weapons)
		{
			if (weapon.IsClassname(name))
			{
				result = weapon;
				return true;
			}
		}

		return false;
	}

	inline bool LookUpWeaponInfoByEconIndex(const int index, WeaponInfo& result) const
	{
		if (index < 0) { return false; }

		for (auto& weapon : m_weapons)
		{
			if (weapon.GetItemDefIndex() == index)
			{
				result = weapon;
				return true;
			}
		}

		return false;
	}

	void PostParseAnalysis();
};

#endif // !NAVBOT_WEAPON_INFO_H_

