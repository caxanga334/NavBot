#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <mods/basemod.h>
#include "weapon.h"

CBotWeapon::CBotWeapon(CBaseEntity* entity) : m_bcw(entity)
{
	edict_t* edict = reinterpret_cast<IServerEntity*>(entity)->GetNetworkable()->GetEdict();

	auto classname = gamehelpers->GetEntityClassname(entity);
	m_econindex = extmanager->GetMod()->GetWeaponEconIndex(edict);
	m_weaponID = extmanager->GetMod()->GetWeaponID(edict);

	m_info = extmanager->GetWeaponInfoManager().GetWeaponInfo(classname, m_econindex);
	UtilHelpers::SetHandleEntity(m_handle, entity);
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
