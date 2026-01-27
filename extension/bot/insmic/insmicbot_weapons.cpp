#include NAVBOT_PCH_FILE
#include <mods/insmic/insmic_lib.h>
#include "insmicbot.h"
#include "insmicbot_weapons.h"

CInsMICBotWeapon::CInsMICBotWeapon(CBaseEntity* entity) :
	CBotWeapon(entity)
{
	DetermineWeaponType(entity);
}

int CInsMICBotWeapon::GetPrimaryAmmoLeft(const CBaseBot* owner) const
{
	int ammo = 0;
	CBaseEntity* weapon = GetEntity();

	switch (m_weaponType)
	{
	case WeaponType::TYPE_LOADABLE:
		entprops->GetEntProp(weapon, Prop_Send, "m_iAmmoCount", ammo);
		ammo += 1; // m_iAmmoCount is 0 when there is only 1 left
		break;
	case WeaponType::TYPE_CLIP:
		entprops->GetEntProp(weapon, Prop_Send, "m_iClip", ammo);
		ammo += insmiclib::GetBallisticWeaponReserveAmmoLeft(weapon);
		break;
	default:
		break;
	}

	return ammo;
}

int CInsMICBotWeapon::GetSecondaryAmmoLeft(const CBaseBot* owner) const
{
	int ammo = 0;

	if (HasGrenadeLauncher())
	{
		entprops->GetEntProp(GetEntity(), Prop_Send, "m_iGrenades", ammo);
	}

	return ammo;
}

bool CInsMICBotWeapon::IsLoaded() const
{
	CBaseEntity* weapon = GetEntity();

	if (HasGrenadeLauncher())
	{
		bool usingGL = false;
		entprops->GetEntPropBool(weapon, Prop_Send, "m_bSightsUp", usingGL);

		if (usingGL)
		{
			bool loaded = false;
			entprops->GetEntPropBool(weapon, Prop_Send, "m_bGrenadeLoaded", loaded);
			return loaded;
		}
	}

	switch (m_weaponType)
	{
	case WeaponType::TYPE_LOADABLE:
	{
		bool loaded = false;
		entprops->GetEntPropBool(weapon, Prop_Send, "m_bLoaded", loaded);
		return loaded;
	}
	case WeaponType::TYPE_CLIP:
	{
		int inclip = 0;
		entprops->GetEntProp(weapon, Prop_Send, "m_iClip", inclip);
		return inclip > 0;
	}
	default:
		break;
	}

	return false;
}

bool CInsMICBotWeapon::IsDeployedOrScoped(const CBaseBot* owner) const
{
	return insmiclib::IsPlayerFlagSet(owner->GetEntity(), insmic::PLAYERFLAG_AIMING);
}

void CInsMICBotWeapon::DetermineWeaponType(CBaseEntity* entity)
{
	if (entprops->HasEntProp(entity, Prop_Send, "m_iAmmoCount"))
	{
		m_weaponType = WeaponType::TYPE_LOADABLE;
	}
	else
	{
		m_weaponType = WeaponType::TYPE_CLIP;
	}

	m_hasGL = entprops->HasEntProp(entity, Prop_Send, "m_iGrenades");
}
