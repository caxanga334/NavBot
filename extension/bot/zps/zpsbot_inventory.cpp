#include NAVBOT_PCH_FILE
#include "zpsbot.h"
#include "zpsbot_inventory.h"


CZPSBotInventory::CZPSBotInventory(CZPSBot* bot) :
	IInventory(bot)
{
	m_lastammosearchweapon = nullptr;
}

void CZPSBotInventory::Reset()
{
	m_lastammosearchweapon = nullptr;
	IInventory::Reset();
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

void CZPSBotInventory::DropHeldWeapon() const
{
	GetBot<CZPSBot>()->DelayedFakeClientCommand("dropweapon");
}

const CBotWeapon* CZPSBotInventory::GetWeaponWithLowAmmo()
{
	CZPSBot* bot = GetBot<CZPSBot>();

	for (auto& weaponptr : GetAllWeapons())
	{
		const CBotWeapon* weapon = weaponptr.get();

		if (weapon->IsValid() && weapon->IsOwnedByBot(bot))
		{
			if (m_lastammosearchweapon == weapon) { continue; }

			if (weapon->IsAmmoLow(bot))
			{
				m_lastammosearchweapon = weapon;
				return weapon;
			}
		}
	}

	m_lastammosearchweapon = nullptr;
	return nullptr;
}

const CZPSBotWeapon* CZPSBotInventory::FindItemDeliver(const std::string& id)
{
	const CZPSBotWeapon* result = nullptr;

	auto func = [&result, &id](const CBotWeapon* base) {
		if (result) { return; }

		const CZPSBotWeapon* weapon = static_cast<const CZPSBotWeapon*>(base);
		
		if (weapon->ClassnameMatchesPattern("item_deliver"))
		{
			std::string itemid = entprops->GetEntPropString(weapon->GetEntity(), Prop_Data, "m_strItemID");

			if (ke::StrCaseCmp(itemid.c_str(), id.c_str()) == 0)
			{
				result = weapon;
			}
		}
	};

	ForEveryWeapon(func);
	return result;
}
