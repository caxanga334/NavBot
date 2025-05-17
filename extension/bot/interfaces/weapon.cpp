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
	m_info = extmanager->GetMod()->GetWeaponInfoManager()->GetWeaponInfo(classname, m_econindex);
	m_handle = entity;
	m_classname.assign(classname);
}

CBotWeapon::~CBotWeapon()
{
}

bool CBotWeapon::IsValid() const
{
	return m_info.get() != nullptr && m_handle.Get() != nullptr;
}

edict_t* CBotWeapon::GetEdict() const
{
	return reinterpret_cast<IServerEntity*>(m_handle.Get())->GetNetworkable()->GetEdict();
}

CBaseEntity* CBotWeapon::GetEntity() const
{
	return m_handle.Get();
}

int CBotWeapon::GetIndex() const
{
	return gamehelpers->EntityToBCompatRef(m_handle.Get());
}

bool CBotWeapon::IsAmmoLow(const CBaseBot* owner, const bool primaryOnly, const int lowThresholdOverride) const
{
	// Also consider melee primary as infinite ammo weapons
	if (m_info->HasInfiniteAmmo() || m_info->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).IsMelee())
	{
		return false;
	}

	bool low = false;
	int ammo = 0;
	int limit = lowThresholdOverride > 0 ? lowThresholdOverride : m_info->GetLowPrimaryAmmoThreshold();

	if (!m_info->Clip1IsReserveAmmo())
	{
		ammo += m_bcw.GetClip1();
	}

	int type = m_bcw.GetPrimaryAmmoType();

	if (type >= 0 && type < MAX_AMMO_TYPES)
	{
		ammo += owner->GetAmmoOfIndex(type);
	}

	if (ammo <= limit)
	{
		return true;
	}

	if (!primaryOnly)
	{
		limit = lowThresholdOverride > 0 ? lowThresholdOverride : m_info->GetLowSecondaryAmmoThreshold();
		ammo = 0;

		if (!m_info->Clip2IsReserveAmmo())
		{
			ammo += m_bcw.GetClip2();
		}

		type = m_bcw.GetSecondaryAmmoType();

		if (type >= 0 && type < MAX_AMMO_TYPES)
		{
			ammo += owner->GetAmmoOfIndex(type);
		}

		if (ammo <= limit)
		{
			return true;
		}
	}

	return false;
}

bool CBotWeapon::IsOutOfAmmo(const CBaseBot* owner, const bool inClipOnly, const bool primaryOnly) const
{
	// Also consider melee primary as infinite ammo weapons
	if (m_info->HasInfiniteAmmo() || m_info->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).IsMelee())
	{
		return false;
	}

	bool low = false;
	int ammo = 0;
	int type = -1;

	if (!m_info->Clip1IsReserveAmmo())
	{
		ammo += m_bcw.GetClip1();
	}

	if (!inClipOnly || m_info->Clip1IsReserveAmmo())
	{
		type = m_bcw.GetPrimaryAmmoType();

		if (type >= 0 && type < MAX_AMMO_TYPES)
		{
			ammo += owner->GetAmmoOfIndex(type);
		}
	}

	if (ammo <= 0)
	{
		return true;
	}

	if (!primaryOnly)
	{
		ammo = 0;

		if (!m_info->Clip2IsReserveAmmo())
		{
			ammo += m_bcw.GetClip2();
		}

		if (!inClipOnly || m_info->Clip2IsReserveAmmo())
		{
			type = m_bcw.GetSecondaryAmmoType();

			if (type >= 0 && type < MAX_AMMO_TYPES)
			{
				ammo += owner->GetAmmoOfIndex(type);
			}
		}

		if (ammo <= 0)
		{
			return true;
		}
	}

	return false;
}

bool CBotWeapon::IsClipFull() const
{
	if (m_info->HasMaxClip1())
	{
		return m_bcw.GetClip1() >= m_info->GetMaxClip1();
	}

	return true;
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
