#include NAVBOT_PCH_FILE
#include <mods/zps/zps_mod.h>
#include "zpsbot.h"
#include "zpsbot_weapon.h"
#include "zpsbot_weaponinfo.h"
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

bool CZPSBotInventory::ShouldDropWeaponToPick(CBaseEntity* entity) const
{
	CZombiePanicSourceMod* mod = CZombiePanicSourceMod::GetZPSMod();
	auto info = mod->GetWeaponInfoManager()->GetWeaponInfo(entity);

	if (info)
	{
		int invsize = GetInventorySize();
		int size = static_cast<const ZPSWeaponInfo*>(info)->GetInventorySize();
		
		if (invsize + size > MAX_INVENTORY_SIZE)
		{
			return true;
		}
	}

	return false;
}

bool CZPSBotInventory::CanPickupAnyWeapons() const
{
	if (FindFirstOutOfAmmoWeapon() == nullptr)
	{
		// if we don't have any empty weapons and we are at max inventory capacity, don't pickup any weapons
		if (GetInventorySize() >= MAX_INVENTORY_SIZE)
		{
			return false;
		}
	}

	return true;
}

bool CZPSBotInventory::CanPickupWeapon(CBaseEntity* entity) const
{
	if (IInventory::CanPickupWeapon(entity))
	{
		// ZPS logic. Limit bots to only one weapon per slot

		CZombiePanicSourceMod* mod = CZombiePanicSourceMod::GetZPSMod();
		auto info = mod->GetWeaponInfoManager()->GetWeaponInfo(entity);

		if (!info) { return false; }

		auto inslot = FindWeaponBySlot(info->GetSlot());

		if (inslot != nullptr)
		{
			// allow if this weapon is out of ammo.
			if (inslot->IsOutOfAmmo(GetBot<CBaseBot>()))
			{
				return true;
			}

			return false;
		}

		return true;
	}

	return false;
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

			// Fallback to targetname for legacy/old maps
			const char* targetname = entityprops::GetEntityTargetname(weapon->GetEntity());

			if (targetname && targetname[0] != '\0')
			{
				if (ke::StrCaseCmp(targetname, id.c_str()) == 0)
				{
					result = weapon;
				}
			}
		}
	};

	ForEveryWeapon(func);
	return result;
}
