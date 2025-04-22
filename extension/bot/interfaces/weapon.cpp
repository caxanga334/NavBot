#include <limits>

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
