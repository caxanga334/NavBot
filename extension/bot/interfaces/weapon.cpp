#include NAVBOT_PCH_FILE
#include <limits>
#include <algorithm>

#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <mods/basemod.h>
#include <bot/basebot.h>
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
	if (!info->IsCombatWeapon() || !info->GetAttackInfo(botweapons::AttackType::PRIMARY).HasFunction())
	{
		return false;
	}

	// If the weapon is primary melee or tagged with infinite reserve ammo, it's never low on ammo.
	if (info->HasInfiniteReserveAmmo() || info->GetAttackInfo(botweapons::AttackType::PRIMARY).IsMelee())
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

	if (info->HasLowSecondaryAmmoThreshold() && !info->SecondaryUsesPrimaryAmmo() && info->GetAttackInfo(botweapons::AttackType::SECONDARY).HasFunction())
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
	if (info->GetAttackInfo(botweapons::AttackType::PRIMARY).IsMelee())
	{
		return false;
	}

	if (info->HasCustomAmmoProperty())
	{
		if (!info->GetCustomAmmoProperty().TestCondition(owner->GetEntity(), GetEntity()))
		{
			return true;
		}

		return false;
	}

	int ammo = 0;
	
	ammo += GetPrimaryAmmoLeft(owner);

	if (!info->SecondaryUsesPrimaryAmmo() && info->GetAttackInfo(botweapons::AttackType::SECONDARY).HasFunction())
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

bool CBotWeapon::IsInAttackRange(const float range, const botweapons::AttackType attackType) const
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

	if (info->GetAttackInfo(botweapons::AttackType::PRIMARY).IsMelee())
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

	if (!info->GetAttackInfo(botweapons::AttackType::SECONDARY).HasFunction())
	{
		return false;
	}

	if (info->GetAttackInfo(botweapons::AttackType::SECONDARY).IsMelee())
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

	if (loaded && !info->Clip2IsReserveAmmo() && info->GetAttackInfo(botweapons::AttackType::SECONDARY).HasFunction() && bcw.UsesClipsForAmmo2() && bcw.GetClip2() == 0)
	{
		loaded = false;
	}

	return loaded;
}

float CBotWeapon::GetCurrentMinimumAttackRange(CBaseBot* owner) const
{
	botweapons::AttackType type = botweapons::GetValidAttackType(owner->GetControlInterface()->GetLastUsedAttackType());
	
	if (m_info->GetAttackInfo(type).HasMinRange())
	{
		return m_info->GetAttackInfo(type).GetMinRange();
	}

	return 0.0f;
}

float CBotWeapon::GetCurrentMaximumAttackRange(CBaseBot* owner) const
{
	botweapons::AttackType type = botweapons::GetValidAttackType(owner->GetControlInterface()->GetLastUsedAttackType());

	if (m_info->GetAttackInfo(type).HasMaxRange())
	{
		return m_info->GetAttackInfo(type).GetMaxRange();
	}

	return std::numeric_limits<float>::max();
}

const int CBotWeapon::GetPriority(const CBaseBot* owner, const CKnownEntity* threat) const
{
	const WeaponInfo* info = GetWeaponInfo();
	int priority = info->GetPriority();
	auto& dynprios = info->GetDynamicPriotities();

	if (dynprios.empty()) { return priority; }
	
	for (const std::unique_ptr<IDynamicWeaponPriority>& dynprio : dynprios)
	{
		priority += dynprio->GetPriorityValue(owner, this, threat);
	}

	return priority;
}

bool CBotWeapon::IsDeployedOrScoped(const CBaseBot* owner) const
{
	const WeaponInfo* info = GetWeaponInfo();

	if (info->HasDeployedStateProperty())
	{
		return info->GetDeployedProperty().TestCondition(owner->GetEntity(), GetEntity());
	}

	return false;
}

bool CBotWeapon::CanUseSpecialFunction(const CBaseBot* owner, const float range) const
{
	const WeaponInfo* info = GetWeaponInfo();
	const WeaponInfo::SpecialFunction& func = info->GetSpecialFunction();

	if (func.IsEnabled())
	{
		if (range < func.min_range || range > func.max_range)
		{
			return false;
		}

		return func.entprop.TestCondition(owner->GetEntity(), GetEntity());
	}

	return false;
}

bool CBotWeapon::CanBeReloaded(const CBaseBot* owner) const
{
	const WeaponInfo* info = GetWeaponInfo();

	if (info->GetAttackInfo(botweapons::AttackType::PRIMARY).IsMelee())
	{
		return false; // if a weapon's primary attack is melee, assume it doesn't need to reload
	}

	// this one is tricky, if a weapon uses the reserve ammo directly, assume it doesn't have clips (IE: TF2's sniper rifle)
	if (info->Clip1IsReserveAmmo())
	{
		return false;
	}

	return true;
}

float CBotWeapon::GetCustomAmmo(const CBaseBot* owner) const
{
	const WeaponEntityProperty& prop = GetWeaponInfo()->GetCustomAmmoProperty();

	if (prop.GetValueType() == WeaponEntityProperty::ValueType::TYPE_FLOAT)
	{
		return prop.ReadPropFloat(owner->GetEntity(), GetEntity());
	}

	return static_cast<float>(prop.ReadPropInt(owner->GetEntity(), GetEntity()));
}

const char* CBotWeapon::GetDebugIdentifier() const
{
	static std::array<char, 512> string;

	ke::SafeSprintf(string.data(), string.size(), "#%i %s (%s:%i)", m_entindex, m_info->GetConfigEntryName(), m_classname.c_str(), m_econindex);

	return string.data();
}

int CBotWeapon::GetSubType() const
{
	// base reads the datamap, if needed a mod can override to an SDKCall of CBaseCombatWeapon::GetSubType (virtual)
	return m_bcw.GetSubTypeFromProperty();
}
