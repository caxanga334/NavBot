#include NAVBOT_PCH_FILE
#include <extension.h>
#include <mods/blackmesa/blackmesadm_mod.h>
#include "bmbot.h"
#include "bmbot_inventory.h"

CBlackMesaBotInventory::CBlackMesaBotInventory(CBlackMesaBot* bot) :
	IInventory(bot)
{
}

CBlackMesaBotInventory::~CBlackMesaBotInventory()
{
}

bool CBlackMesaBotInventory::OwnsThisWeapon(const std::string& itemname)
{
	std::size_t it = itemname.find("item_");

	if (it == std::string::npos)
	{
		return false;
	}

	constexpr std::size_t item_length = 5U; // length of item_
	std::string weaponname = itemname.substr(it + 5U);
	return HasWeapon(weaponname.c_str());
}

blackmesa::BMAmmoIndex CBlackMesaBotInventory::SelectRandomNeededAmmo() const
{
	std::vector<blackmesa::BMAmmoIndex> ammos;
	CBaseBot* me = GetBot();
	CBlackMesaDeathmatchMod* mod = CBlackMesaDeathmatchMod::GetBMMod();

	for (auto& weapon : GetAllWeapons())
	{
		if (weapon->IsValid())
		{
			const int lowprimary = weapon->GetWeaponInfo()->GetLowPrimaryAmmoThreshold();
			const int lowsecondary = weapon->GetWeaponInfo()->GetLowSecondaryAmmoThreshold();


			int index = weapon->GetBaseCombatWeapon().GetPrimaryAmmoType();

			if (index > 0)
			{
				int quantity = me->GetAmmoOfIndex(index);

				if (quantity <= lowprimary)
				{
					ammos.push_back(static_cast<blackmesa::BMAmmoIndex>(index));
				}
			}

			index = weapon->GetBaseCombatWeapon().GetSecondaryAmmoType();

			if (index > 0)
			{
				int quantity = me->GetAmmoOfIndex(index);

				if (quantity <= lowsecondary)
				{
					ammos.push_back(static_cast<blackmesa::BMAmmoIndex>(index));
				}
			}
		}
	}

	if (ammos.empty())
	{
		return blackmesa::BMAmmoIndex::Ammo_Invalid;
	}

	if (ammos.size() == 1)
	{
		return ammos[0];
	}

	blackmesa::BMAmmoIndex result = librandom::utils::GetRandomElementFromVector(ammos);

	return result;
}
