#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <entities/basecombatweapon.h>

entities::HBaseCombatWeapon::HBaseCombatWeapon(edict_t* entity) : HBaseEntity(entity)
{
}

entities::HBaseCombatWeapon::HBaseCombatWeapon(CBaseEntity* entity) : HBaseEntity(entity)
{
}

int entities::HBaseCombatWeapon::GetClip1() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iClip1", value);
	return value;
}

int entities::HBaseCombatWeapon::GetClip2() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iClip2", value);
	return value;
}

bool entities::HBaseCombatWeapon::UsesClipsForAmmo1() const
{
	// The game function calls maxclip, this function is virtual so we could SDKCall it.
	// This method should work as long third-party plugins doesn't mess with the weapon's m_iClip1 property.
	// Needs to be Prop_Data since the networked variable is networked as an unsigned char.

	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_iClip1", value);
	return value != WEAPON_NOCLIP;
}

bool entities::HBaseCombatWeapon::UsesClipsForAmmo2() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_iClip2", value);
	return value != WEAPON_NOCLIP;
}

int entities::HBaseCombatWeapon::GetPrimaryAmmoType() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iPrimaryAmmoType", value);
	return value;
}

int entities::HBaseCombatWeapon::GetSecondaryAmmoType() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iSecondaryAmmoType", value);
	return value;
}

int entities::HBaseCombatWeapon::GetPrimaryAmmoCount() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_iPrimaryAmmoCount", value);
	return value;
}

int entities::HBaseCombatWeapon::GetSecondaryAmmoCount() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_iSecondaryAmmoCount", value);
	return value;
}

int entities::HBaseCombatWeapon::GetState() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iState", value);
	return value;
}

int entities::HBaseCombatWeapon::GetOwnerIndex() const
{
	int owner = INVALID_EHANDLE_INDEX;
	entprops->GetEntPropEnt(GetIndex(), Prop_Send, "m_hOwner", &owner);
	return owner;
}

edict_t* entities::HBaseCombatWeapon::GetOwner() const
{
	int owner = INVALID_EHANDLE_INDEX;
	entprops->GetEntPropEnt(GetIndex(), Prop_Send, "m_hOwner", &owner);
	return gamehelpers->EdictOfIndex(owner);
}

bool entities::HBaseCombatWeapon::IsOnGround() const
{
	return GetState() == WEAPON_NOT_CARRIED;
}

bool entities::HBaseCombatWeapon::IsCarried() const
{
	return GetState() == WEAPON_IS_CARRIED_BY_PLAYER;
}

bool entities::HBaseCombatWeapon::IsActive() const
{
	return GetState() == WEAPON_IS_ACTIVE;
}
