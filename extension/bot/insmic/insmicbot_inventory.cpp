#include NAVBOT_PCH_FILE
#include "insmicbot.h"
#include "insmicbot_weapons.h"
#include "insmicbot_inventory.h"

CInsMICBotInventory::CInsMICBotInventory(CInsMICBot* bot) :
	IInventory(bot)
{
}

void CInsMICBotInventory::BuildInventory()
{
	// fully overriden for insurgency since CBaseCombatCharacter doesn't exists

	static std::size_t size = 0;
	CInsMICBot* bot = GetBot<CInsMICBot>();
	SetActiveWeaponCache(nullptr);

	if (size == 0)
	{
		size = entprops->GetEntPropArraySize(bot->GetIndex(), Prop_Send, "m_hMyWeapons");
	}

	for (std::size_t i = 0; i < size; i++)
	{
		CBaseEntity* weapon = nullptr;
		entprops->GetEntPropEnt(bot->GetEntity(), Prop_Send, "m_hMyWeapons", nullptr, &weapon, static_cast<int>(i));

		if (weapon)
		{
			AddWeaponToInventory(weapon);
		}
	}
}

bool CInsMICBotInventory::EquipWeapon(const CBotWeapon* weapon) const
{
	const CBotWeapon* activeWeapon = GetActiveBotWeapon();

	if (activeWeapon != nullptr && activeWeapon == weapon)
	{
		return false;
	}

	GetBot<CInsMICBot>()->SelectWeaponByUserCmd(weapon->GetIndex());
	OnBotWeaponEquipped(weapon);
	return true;
}

bool CInsMICBotInventory::EquipWeapon(CBaseEntity* weapon) const
{
	CInsMICBot* me = GetBot<CInsMICBot>();
	CBaseEntity* active = me->GetActiveWeapon();

	if (active == weapon)
	{
		return false;
	}

	me->SelectWeaponByUserCmd(UtilHelpers::IndexOfEntity(weapon));
	const CBotWeapon* botWeapon = GetWeaponOfEntity(weapon);

	if (botWeapon)
	{
		OnBotWeaponEquipped(botWeapon);
	}

	return true;
}

CBotWeapon* CInsMICBotInventory::CreateBotWeapon(CBaseEntity* weapon)
{
	return new CInsMICBotWeapon(weapon);
}
