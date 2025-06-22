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

#endif // EXT_DEBUG
}

CBotWeapon::~CBotWeapon()
{
}

bool CBotWeapon::IsValid() const
{
	return m_info != nullptr && m_handle.Get() != nullptr;
}

edict_t* CBotWeapon::GetEdict() const
{
	return reinterpret_cast<IServerEntity*>(m_handle.Get())->GetNetworkable()->GetEdict();
}

bool CBotWeapon::IsAmmoLow(const CBaseBot* owner) const
{
	/* 
	 * This function is used by the bot to determine if they are low on ammo and should fetch ammo. 
	 * It should return true if the bot needs to fetch ammo for this weapon.
	*/

	auto info = GetWeaponInfo();
	auto& bcw = GetBaseCombatWeapon();

	// Ignore weapons that doesn't have a primary attack
	if (!info->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).HasFunction())
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

	if (info->GetDynamicPriorityHasSecondaryAmmo() != 0 && hasSecondaryAmmo(owner))
	{
		priority += info->GetDynamicPriorityHasSecondaryAmmo();
	}

	if (info->GetDynamicPriorityHealthPercentageCondition() > 0.0f && owner->GetHealthPercentage() <= info->GetDynamicPriorityHealthPercentageCondition())
	{
		priority += info->GetDynamicPriorityHealthPercentage();
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
