#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <mods/basemod.h>
#include "weapon.h"

CBotWeapon::CBotWeapon(edict_t* entity) : m_bcw(entity)
{
	auto classname = gamehelpers->GetEntityClassname(entity);
	m_econindex = extmanager->GetMod()->GetWeaponEconIndex(entity);
	m_weaponID = extmanager->GetMod()->GetWeaponID(entity);

	m_info = extmanager->GetWeaponInfoManager().GetWeaponInfo(classname, m_econindex);
	UtilHelpers::SetHandleEntity(m_handle, entity);
}

CBotWeapon::~CBotWeapon()
{
}

bool CBotWeapon::IsValid() const
{
	return m_handle.IsValid() && (UtilHelpers::GetEdictFromCBaseHandle(m_handle) != nullptr);
}

edict_t* CBotWeapon::GetEdict() const
{
	return UtilHelpers::GetEdictFromCBaseHandle(m_handle);
}

CBaseEntity* CBotWeapon::GetEntity() const
{
	return UtilHelpers::GetBaseEntityFromCBaseHandle(m_handle);
}
