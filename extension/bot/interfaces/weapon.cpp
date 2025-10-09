#include NAVBOT_PCH_FILE
#include <limits>
#include <algorithm>

#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <mods/basemod.h>
#include "weapon.h"

// undef some macros from mathlib that causes conflicts with std::
#undef min
#undef max
#undef clamp

CBotWeapon::CBotWeapon(CBaseEntity* entity) : m_bcw(entity)
{
	edict_t* edict = reinterpret_cast<IServerEntity*>(entity)->GetNetworkable()->GetEdict();

	auto classname = entityprops::GetEntityClassname(entity);
	m_econindex = extmanager->GetMod()->GetWeaponEconIndex(edict);
	m_entindex = gamehelpers->EntityToBCompatRef(entity);
	m_info = extmanager->GetMod()->GetWeaponInfoManager()->GetWeaponInfo(classname, m_econindex);
	m_handle = entity;
	m_classname.assign(classname);

#ifdef EXT_DEBUG
	
	if (m_info != nullptr && m_info->IsCombatWeapon())
	{
		int clip = 0;
		entprops->GetEntProp(m_entindex, Prop_Data, "m_iClip1", clip);

		if (clip == WEAPON_NOCLIP && !m_info->Clip1IsReserveAmmo() && !m_info->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).IsMelee())
		{
			smutils->LogError(myself, "Weapon %s CLIP1 == WEAPON_NOCLIP but lacks \"primary_no_clip\" attribute. %s #%i",
				m_classname.c_str(), m_info->GetConfigEntryName(), m_econindex);
		}

		clip = 0;
		entprops->GetEntProp(m_entindex, Prop_Data, "m_iClip2", clip);

		if (clip == WEAPON_NOCLIP && !m_info->Clip2IsReserveAmmo() && m_info->GetAttackInfo(WeaponInfo::AttackFunctionType::SECONDARY_ATTACK).HasFunction() && !m_info->GetAttackInfo(WeaponInfo::AttackFunctionType::SECONDARY_ATTACK).IsMelee())
		{
			smutils->LogError(myself, "Weapon %s CLIP2 == WEAPON_NOCLIP but lacks \"secondary_no_clip\" attribute. %s #%i",
				m_classname.c_str(), m_info->GetConfigEntryName(), m_econindex);
		}
	}

#endif // EXT_DEBUG
}

CBotWeapon::~CBotWeapon()
{
}

bool CBotWeapon::IsValid() const
{
	return m_info != nullptr && m_handle.Get() != nullptr;
}

bool CBotWeapon::IsOwnedByBot(const CBaseBot* bot) const
{
	if (m_handle.Get() != nullptr)
	{
		return m_bcw.GetOwnerIndex() == bot->GetIndex();
	}

	return false;
}

edict_t* CBotWeapon::GetEdict() const
{
	return reinterpret_cast<IServerEntity*>(m_handle.Get())->GetNetworkable()->GetEdict();
}

bool CBotWeapon::IsWeapon(const char* classname) const
{
	return std::strcmp(m_classname.c_str(), classname) == 0;
}

const bool CBotWeapon::ClassnameMatchesPattern(const char* pattern) const
{
	return UtilHelpers::StringMatchesPattern(m_classname.c_str(), pattern, 0);
}

bool CBotWeapon::IsAmmoLow(const CBaseBot* owner) const
{
	/* 
	 * This function is used by the bot to determine if they are low on ammo and should fetch ammo. 
	 * It should return true if the bot needs to fetch ammo for this weapon.
	*/

	auto info = GetWeaponInfo();
	auto& bcw = GetBaseCombatWeapon();

	// Ignore weapons that doesn't have a primary attack or aren't a combat weapon
	if (!info->IsCombatWeapon() || !info->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).HasFunction())
	{
		return false;
	}

	// If the weapon is primary melee or tagged with infinite reserve ammo, it's never low on ammo.
	if (info->HasInfiniteReserveAmmo() || info->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).IsMelee())
	{
		return false;
	}

	if (info->HasCustomAmmoProperty())
	{
		return static_cast<int>(GetCustomAmmo(owner)) <= info->GetLowPrimaryAmmoThreshold();
	}

	if (info->HasLowPrimaryAmmoThreshold())
	{
		int ammo = 0;

		if (!info->Clip1IsReserveAmmo())
		{
			ammo += bcw.GetClip1();
		}

		int type = bcw.GetPrimaryAmmoType();

		if (type > 0 && type <= MAX_AMMO_TYPES)
		{
			ammo += owner->GetAmmoOfIndex(type);
		}

		if (ammo <= info->GetLowPrimaryAmmoThreshold())
		{
			return true;
		}
	}

	if (info->HasLowSecondaryAmmoThreshold() && !info->SecondaryUsesPrimaryAmmo() && info->GetAttackInfo(WeaponInfo::AttackFunctionType::SECONDARY_ATTACK).HasFunction())
	{
		int ammo = 0;

		if (!info->Clip2IsReserveAmmo())
		{
			ammo += bcw.GetClip2();
		}

		int type = bcw.GetSecondaryAmmoType();

		if (type > 0 && type <= MAX_AMMO_TYPES)
		{
			ammo += owner->GetAmmoOfIndex(type);
		}

		if (ammo <= info->GetLowSecondaryAmmoThreshold())
		{
			return true;
		}
	}

	return false;
}

bool CBotWeapon::IsOutOfAmmo(const CBaseBot* owner) const
{
	// This function checks if the weapon is completely out of ammo

	auto info = GetWeaponInfo();

	// primary melee never runs out of ammo
	if (info->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).IsMelee())
	{
		return false;
	}

	if (info->HasCustomAmmoProperty())
	{
		if (GetCustomAmmo(owner) <= info->GetCustomAmmoOutOfAmmoThreshold())
		{
			return true;
		}

		return false;
	}

	int ammo = 0;
	
	ammo += GetPrimaryAmmoLeft(owner);

	if (!info->SecondaryUsesPrimaryAmmo() && info->GetAttackInfo(WeaponInfo::AttackFunctionType::SECONDARY_ATTACK).HasFunction())
	{
		ammo += GetSecondaryAmmoLeft(owner);
	}

	return ammo <= 0;
}

int CBotWeapon::GetPrimaryAmmoLeft(const CBaseBot* owner) const
{
	int ammo = 0;

	int type = GetBaseCombatWeapon().GetPrimaryAmmoType();

	if (type > 0 && type < MAX_AMMO_TYPES)
	{
		ammo += owner->GetAmmoOfIndex(type);
	}

	// Uses clips
	if (!GetWeaponInfo()->Clip1IsReserveAmmo())
	{
		ammo += GetBaseCombatWeapon().GetClip1();
	}

	return ammo;
}

int CBotWeapon::GetSecondaryAmmoLeft(const CBaseBot* owner) const
{
	if (GetWeaponInfo()->SecondaryUsesPrimaryAmmo())
	{
		return GetPrimaryAmmoLeft(owner);
	}

	int ammo = 0;

	int type = GetBaseCombatWeapon().GetSecondaryAmmoType();

	if (type > 0 && type < MAX_AMMO_TYPES)
	{
		ammo += owner->GetAmmoOfIndex(type);
	}

	// Uses clips
	if (!GetWeaponInfo()->Clip2IsReserveAmmo())
	{
		ammo += GetBaseCombatWeapon().GetClip2();
	}

	return ammo;
}

bool CBotWeapon::IsInAttackRange(const float range, const WeaponInfo::AttackFunctionType attackType) const
{
	auto& info = GetWeaponInfo()->GetAttackInfo(attackType);

	if (info.HasMaxRange() && range > info.GetMaxRange())
	{
		// larger than max range
		return false;
	}

	if (info.HasMinRange() && range < info.GetMinRange())
	{
		// smaller than min range
		return false;
	}

	return true;
}

bool CBotWeapon::CanUsePrimaryAttack(const CBaseBot* owner) const
{
	const WeaponInfo* info = GetWeaponInfo();

	if (info->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).IsMelee())
	{
		return true;
	}

	return HasPrimaryAmmo(owner);
}

bool CBotWeapon::CanUseSecondaryAttack(const CBaseBot* owner) const
{
	const WeaponInfo* info = GetWeaponInfo();

	if (info->GetChanceToUseSecondaryAttack() == 0)
	{
		return false;
	}

	if (!info->GetAttackInfo(WeaponInfo::AttackFunctionType::SECONDARY_ATTACK).HasFunction())
	{
		return false;
	}

	if (info->GetAttackInfo(WeaponInfo::AttackFunctionType::SECONDARY_ATTACK).IsMelee())
	{
		return true;
	}

	return hasSecondaryAmmo(owner);
}

bool CBotWeapon::IsLoaded() const
{
	const WeaponInfo* info = GetWeaponInfo();
	const entities::HBaseCombatWeapon& bcw = GetBaseCombatWeapon();
	bool loaded = true;

	if (!info->Clip1IsReserveAmmo() && bcw.UsesClipsForAmmo1() && bcw.GetClip1() == 0)
	{
		loaded = false;
	}

	if (loaded && !info->Clip2IsReserveAmmo() && info->GetAttackInfo(WeaponInfo::AttackFunctionType::SECONDARY_ATTACK).HasFunction() && bcw.UsesClipsForAmmo2() && bcw.GetClip2() == 0)
	{
		loaded = false;
	}

	return loaded;
}

float CBotWeapon::GetCurrentMinimumAttackRange(CBaseBot* owner) const
{
	switch (owner->GetControlInterface()->GetLastUsedAttackType())
	{
	case IPlayerInput::AttackType::ATTACK_SECONDARY:
	{
		if (m_info->GetAttackInfo(WeaponInfo::AttackFunctionType::SECONDARY_ATTACK).HasMinRange())
		{
			return m_info->GetAttackInfo(WeaponInfo::AttackFunctionType::SECONDARY_ATTACK).GetMinRange();
		}
		break;
	}
	case IPlayerInput::AttackType::ATTACK_PRIMARY:
		[[fallthrough]];
	default:
	{
		if (m_info->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).HasMinRange())
		{
			return m_info->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).GetMinRange();
		}
		break;
	}
	}

	return 0.0f;
}

float CBotWeapon::GetCurrentMaximumAttackRange(CBaseBot* owner) const
{
	switch (owner->GetControlInterface()->GetLastUsedAttackType())
	{
	case IPlayerInput::AttackType::ATTACK_SECONDARY:
	{
		if (m_info->GetAttackInfo(WeaponInfo::AttackFunctionType::SECONDARY_ATTACK).HasMaxRange())
		{
			return m_info->GetAttackInfo(WeaponInfo::AttackFunctionType::SECONDARY_ATTACK).GetMaxRange();
		}
		break;
	}
	case IPlayerInput::AttackType::ATTACK_PRIMARY:
		[[fallthrough]];
	default:
	{
		if (m_info->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).HasMaxRange())
		{
			return m_info->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).GetMaxRange();
		}
		break;
	}
	}

	return std::numeric_limits<float>::max();
}

const int CBotWeapon::GetPriority(const CBaseBot* owner, const float* range, const CKnownEntity* threat) const
{
	const WeaponInfo* info = GetWeaponInfo();
	int priority = info->GetPriority();

	if (info->GetDynamicPrioritySecAmmo().is_used && hasSecondaryAmmo(owner))
	{
		priority += info->GetDynamicPrioritySecAmmo().priority_value;
	}

	const WeaponInfo::DynamicPriority& healthprio = info->GetDynamicPriorityHealth();

	if (healthprio.is_used)
	{
		if (healthprio.is_greater)
		{
			if (owner->GetHealthPercentage() > healthprio.value_to_compare)
			{
				priority += healthprio.priority_value;
			}
		}
		else
		{
			if (owner->GetHealthPercentage() < healthprio.value_to_compare)
			{
				priority += healthprio.priority_value;
			}
		}
	}

	const WeaponInfo::DynamicPriority& rangeprio = info->GetDynamicPriorityRange();

	if (range && rangeprio.is_used)
	{
		if (rangeprio.is_greater)
		{
			if (rangeprio.value_to_compare > *range)
			{
				priority += rangeprio.priority_value;
			}
		}
		else
		{
			if (rangeprio.value_to_compare < *range)
			{
				priority += rangeprio.priority_value;
			}
		}
	}

	const WeaponInfo::DynamicPriority& aggressionprio = info->GetDynamicPriorityAggression();

	if (aggressionprio.is_used)
	{
		int value = static_cast<int>(std::round(aggressionprio.value_to_compare));

		if (aggressionprio.is_greater)
		{
			if (owner->GetDifficultyProfile()->GetAggressiveness() > value)
			{
				priority += aggressionprio.priority_value;
			}
		}
		else
		{
			if (owner->GetDifficultyProfile()->GetAggressiveness() < value)
			{
				priority += aggressionprio.priority_value;
			}
		}
	}

	return priority;
}

bool CBotWeapon::IsDeployedOrScoped(const CBaseBot* owner) const
{
	const WeaponInfo* info = GetWeaponInfo();

	if (info->HasDeployedStateProperty())
	{
		int source = info->IsDeployedPropertyOnTheWeapon() ? this->GetIndex() : owner->GetIndex();
		bool deployed = false;
		entprops->GetEntPropBool(source, Prop_Send, info->GetDeployedPropertyName().c_str(), deployed);
		return deployed;
	}

	return false;
}

bool CBotWeapon::CanUseSpecialFunction(const CBaseBot* owner, const float range) const
{
	const WeaponInfo* info = GetWeaponInfo();
	const WeaponInfo::SpecialFunction& func = info->GetSpecialFunction();

	if (!func.property_name.empty())
	{
		if (range < func.min_range || range > func.max_range)
		{
			return false;
		}

		int entsource = func.property_on_weapon ? this->GetIndex() : owner->GetIndex();

		if (func.property_is_float)
		{
			float value = 0.0f;
			entprops->GetEntPropFloat(entsource, Prop_Send, func.property_name.c_str(), value);
			return value >= func.available_threshold;
		}
		else
		{
			int value = 0;
			entprops->GetEntProp(entsource, Prop_Send, func.property_name.c_str(), value);
			return value >= static_cast<int>(func.available_threshold);
		}
	}

	return false;
}

float CBotWeapon::GetCustomAmmo(const CBaseBot* owner) const
{
	const WeaponInfo* info = GetWeaponInfo();
	PropType type = info->IsCustomAmmoPropertyNetworked() ? Prop_Send : Prop_Data;
	float result = 0.0f;
	int entity = info->IsCustomAmmoPropertyOnWeapon() ? this->GetIndex() : owner->GetIndex();

	if (info->IsCustomAmmoPropertyAFloat())
	{
		float value = 0.0f;
		entprops->GetEntPropFloat(entity, type, info->GetCustomAmmoPropertyName().c_str(), value);
		result = value;
	}
	else
	{
		int value = 0;
		entprops->GetEntProp(entity, type, info->GetCustomAmmoPropertyName().c_str(), value);
		result = static_cast<float>(value);
	}

	return result;
}

const char* CBotWeapon::GetDebugIdentifier() const
{
	static std::array<char, 512> string;

	ke::SafeSprintf(string.data(), string.size(), "#%i %s (%s:%i)", m_entindex, m_info->GetConfigEntryName(), m_classname.c_str(), m_econindex);

	return string.data();
}