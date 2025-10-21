#include NAVBOT_PCH_FILE
#include "zpsbot.h"
#include "zpsbot_inventory.h"


CZPSBotInventory::CZPSBotInventory(CZPSBot* bot) :
	IInventory(bot)
{
}

bool CZPSBotInventory::EquipWeapon(const CBotWeapon* weapon) const
{
	const CBotWeapon* activeWeapon = GetActiveBotWeapon();

	if (activeWeapon != nullptr && activeWeapon == weapon)
	{
		return false;
	}

	CZPSBot* me = GetBot<CZPSBot>();

	// The weapon switch SDKCall breaks ZPS, switch weapon using the usercmd and let the game handle it
	me->SelectWeaponByUserCmd(weapon->GetIndex(), weapon->GetSubType());
	OnBotWeaponEquipped(weapon);

	return true;
}

bool CZPSBotInventory::EquipWeapon(CBaseEntity* weapon) const
{
	CZPSBot* me = GetBot<CZPSBot>();

	CBaseEntity* active = me->GetActiveWeapon();

	if (active == weapon)
	{
		return false;
	}

	int entindex = UtilHelpers::IndexOfEntity(weapon);
	int subtype = 0;
	entprops->GetEntProp(weapon, Prop_Data, "m_iSubType", subtype);

	me->SelectWeaponByUserCmd(entindex, subtype);
	const CBotWeapon* botWeapon = GetWeaponOfEntity(weapon);

	if (botWeapon)
	{
		OnBotWeaponEquipped(botWeapon);
	}

	return true;
}
