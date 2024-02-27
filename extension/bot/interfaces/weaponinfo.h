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

	inline bool CanHeadShot() const { return can_headshot; }
	inline float GetHeadShotRangeMultiplier() const { return headshot_range_mult; }
	inline float GetMaxPrimaryHeadShotRange() const { return attacksinfo[PRIMARY_ATTACK].GetMaxRange() * headshot_range_mult; }

	inline void SetEconItemIndex(int index) { econindex = index; }
	inline void SetPriority(int pri) { priority = pri; }
	inline void SetMaxClip1(int clip) { maxclip1 = clip; }

	inline bool HasEconIndex() const { return econindex >= 0; }
	inline bool IsEntry(std::string& entry) const { return configentry == entry; }
	inline bool IsClassname(std::string& name) const { return classname == name; }
	inline bool HasPrimaryAttack() const { return attacksinfo[PRIMARY_ATTACK].HasFunction(); }
	inline bool HasSecondaryAttack() const { return attacksinfo[SECONDARY_ATTACK].HasFunction(); }
	inline bool HasTertiaryAttack() const { return attacksinfo[TERTIARY_ATTACK].HasFunction(); }
	inline bool IsCombatWeapon() const { return priority >= 0; }
	inline bool HasMaxClip1() const { return maxclip1 > 0; }
	inline int GetMaxClip1() const { return maxclip1; }

private:
	std::string classname;
	std::string configentry;
	WeaponAttackFunctionInfo attacksinfo[MAX_WEAPON_ATTACKS];
	int econindex; // Economy item definition index
	int priority; // Priority for weapon selection
	bool can_headshot;
	float headshot_range_mult;
	int maxclip1; // Maximum ammo stored in clip1
};

class CWeaponInfoManager : public SourceMod::ITextListener_SMC
{
public:
	inline CWeaponInfoManager() :
		m_tempweapinfo()
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
	 * @brief Searches for a weapon info. Economy index is searched first, then classname.
	 * @param classname Weapon classname to search for.
	 * @param index Weapon economy index to search for.
	 * @return Pointer to WeaponInfo or NULL if not found.
	 */
	inline const WeaponInfo* GetWeaponInfo(const char* classname, const int index) const
	{
		const WeaponInfo* info = GetWeaponInfoByEconIndex(index);

		if (info)
			return info;

		return GetWeaponInfoByClassname(classname);
	}

	/**
	 * @brief Searches for a weapon info by classname
	 * @param classname Weapon classname to search for
	 * @return Pointer to WeaponInfo or NULL if not found
	*/
	inline const WeaponInfo* GetWeaponInfoByClassname(const char* classname) const
	{
		std::string name(classname);

		for (auto& weapon : m_weapons)
		{
			if (weapon.IsClassname(name))
			{
				return &weapon;
			}
		}

		return nullptr;
	}

	/**
	 * @brief Searches for a weapon info by item definition index
	 * @param index Item definition index
	 * @return Pointer to WeaponInfo or NULL if not found
	*/
	inline const WeaponInfo* GetWeaponInfoByEconIndex(const int index) const
	{
		if (index < 0) { return nullptr; }

		for (auto& weapon : m_weapons)
		{
			if (weapon.GetItemDefIndex() == index)
			{
				return &weapon;
			}
		}

		return nullptr;
	}

	inline bool IsWeaponInfoLoaded() const { return m_weapons.size() > 0; }

	/**
	 * @brief Called when entering a new section
	 *
	 * @param states		Parsing states.
	 * @param name			Name of section, with the colon omitted.
	 * @return				SMCResult directive.
	 */
	virtual SMCResult ReadSMC_NewSection(const SMCStates* states, const char* name) override;

	/**
	 * @brief Called when encountering a key/value pair in a section.
	 *
	 * @param states		Parsing states.
	 * @param key			Key string.
	 * @param value			Value string.  If no quotes were specified, this will be NULL,
	 *						and key will contain the entire string.
	 * @return				SMCResult directive.
	 */
	virtual SMCResult ReadSMC_KeyValue(const SMCStates* states, const char* key, const char* value) override;

	/**
	 * @brief Called when leaving the current section.
	 *
	 * @param states		Parsing states.
	 * @return				SMCResult directive.
	 */
	virtual SMCResult ReadSMC_LeavingSection(const SMCStates* states) override;

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

	void PostParseAnalysis();
};

#endif // !NAVBOT_WEAPON_INFO_H_

