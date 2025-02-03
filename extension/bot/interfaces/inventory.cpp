#include <limits>
#include <algorithm>
#include <extension.h>
#include <manager.h>
#include <mods/basemod.h>
#include <util/entprops.h>
#include <bot/basebot.h>
#include "behavior.h"
#include "knownentity.h"
#include "inventory.h"

// undef some stuff from valve's mathlib that conflicts with STL
#undef min
#undef max
#undef clamp

constexpr auto UPDATE_WEAPONS_INTERVAL_AFTER_RESET = 0.1f;

IInventory::IInventory(CBaseBot* bot) : IBotInterface(bot)
{
	m_weapons.reserve(MAX_WEAPONS);
	m_updateWeaponsTimer.Start(UPDATE_WEAPONS_INTERVAL_AFTER_RESET);
	m_cachedActiveWeapon = nullptr;
}

IInventory::~IInventory()
{
}

void IInventory::Reset()
{
	m_weapons.clear();
	m_weaponSwitchCooldown.Invalidate();
	m_updateWeaponsTimer.Start(UPDATE_WEAPONS_INTERVAL_AFTER_RESET);
	m_cachedActiveWeapon = nullptr;
}

void IInventory::Update()
{
	if (m_updateWeaponsTimer.IsElapsed())
	{
		m_updateWeaponsTimer.Start(extmanager->GetMod()->GetModSettings()->GetInventoryUpdateRate());
		BuildInventory();
	}
}

void IInventory::Frame()
{
}

void IInventory::OnWeaponInfoConfigReloaded()
{
	m_weapons.clear();
	m_cachedActiveWeapon = nullptr;
	BuildInventory();
}

bool IInventory::HasWeapon(const char* classname)
{
	return std::any_of(std::begin(m_weapons), std::end(m_weapons), [&classname](const std::shared_ptr<CBotWeapon>& weaponptr) {
		return weaponptr->IsValid() && (strcmp(weaponptr->GetClassname().c_str(), classname) == 0);
	});
}

bool IInventory::HasWeapon(CBaseEntity* weapon)
{
	return std::any_of(std::begin(m_weapons), std::end(m_weapons), [&weapon](const std::shared_ptr<CBotWeapon>& weaponptr) {
		return weaponptr->GetEntity() == weapon;
	});
}

CBaseEntity* IInventory::GetWeaponEntity(const char* classname)
{
	auto it = std::find_if(std::begin(m_weapons), std::end(m_weapons), [&classname](const std::shared_ptr<CBotWeapon>& weaponptr) {
		return weaponptr->IsValid() && (strcmp(weaponptr->GetClassname().c_str(), classname) == 0);
	});

	if (it != m_weapons.end())
	{
		return it->get()->GetEntity();
	}

	return nullptr;
}

void IInventory::AddWeaponToInventory(CBaseEntity* weapon)
{
	if (!HasWeapon(weapon))
	{
		m_weapons.emplace_back(CreateBotWeapon(weapon));
	}
}

void IInventory::BuildInventory()
{
	m_weapons.clear();

	// array CHandle<CBaseEntity> weapons[MAX_WEAPONS]
	CHandle<CBaseEntity>* myweapons = entprops->GetCachedDataPtr<CHandle<CBaseEntity>>(GetBot()->GetEntity(), CEntPropUtils::CacheIndex::CBASECOMBATCHARACTER_MYWEAPONS);

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		CBaseEntity* weapon = myweapons[i].Get();

		if (weapon == nullptr)
		{
			continue;
		}

		// IsValidEdict checks if GetIServerEntity return is not NULL
		m_weapons.emplace_back(CreateBotWeapon(weapon));
	}
}

void IInventory::SelectBestWeaponForThreat(const CKnownEntity* threat)
{
	if (m_weaponSwitchCooldown.HasStarted() && !m_weaponSwitchCooldown.IsElapsed())
	{
		return;
	}

	if (m_weapons.empty())
	{
		BuildInventory();
		m_weaponSwitchCooldown.Start(0.250f);
		return;
	}

	CBaseBot* bot = GetBot();
	const float rangeToThreat = bot->GetRangeTo(threat->GetEdict());
	const CBotWeapon* best = nullptr;
	int priority = std::numeric_limits<int>::min();

	ForEveryWeapon([&bot, &priority, &rangeToThreat, &best](const CBotWeapon* weapon) {
		
		if (!weapon->IsValid())
		{
			return;
		}

		// weapon must be usable against enemies
		if (!weapon->GetWeaponInfo()->IsCombatWeapon())
		{
			return;
		}

		CBaseEntity* pWeapon = weapon->GetEntity();
		CBaseEntity* owner = entprops->GetCachedDataPtr<CHandle<CBaseEntity>>(pWeapon, CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_OWNER)->Get();

		// I must be the weapon's owner
		if (bot->GetEntity() != owner)
		{
			return;
		}

		if (!weapon->GetWeaponInfo()->HasInfiniteAmmo())
		{
			int clip1 = entprops->GetCachedData<int>(pWeapon, CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_CLIP1);
			int primary_ammo_index = entprops->GetCachedData<int>(pWeapon, CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_PRIMARYAMMOTYPE);

			// Must have ammo
			if (clip1 == 0 && bot->GetAmmoOfIndex(primary_ammo_index) == 0)
			{
				return;
			}
		}

		if (rangeToThreat > weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK).GetMaxRange())
		{
			return; // outside max range
		}
		else if (rangeToThreat < weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK).GetMinRange())
		{
			return; // too close
		}

		int currentprio = weapon->GetWeaponInfo()->GetPriority();

		if (currentprio > priority)
		{
			best = weapon;
			priority = currentprio;
		}
	});

	m_weaponSwitchCooldown.Start(base_weapon_switch_cooldown());

	if (best != nullptr)
	{
		if (bot->GetBehaviorInterface()->ShouldSwitchToWeapon(bot, best) != ANSWER_NO)
		{
			bot->SelectWeapon(best->GetEntity());
		}
	}
}

void IInventory::SelectBestWeapon()
{
	if (m_weaponSwitchCooldown.HasStarted() && !m_weaponSwitchCooldown.IsElapsed())
	{
		return;
	}

	if (m_weapons.empty())
	{
		BuildInventory();
		m_weaponSwitchCooldown.Start(0.250f);
		return;
	}

	CBaseBot* bot = GetBot();
	const CBotWeapon* best = nullptr;
	int priority = std::numeric_limits<int>::min();

	ForEveryWeapon([&bot, &priority, &best](const CBotWeapon* weapon) {

		if (!weapon->IsValid())
		{
			return;
		}

		// weapon must be usable against enemies
		if (!weapon->GetWeaponInfo()->IsCombatWeapon())
		{
			return;
		}

		CBaseEntity* pWeapon = weapon->GetEntity();
		CBaseEntity* owner = entprops->GetCachedDataPtr<CHandle<CBaseEntity>>(pWeapon, CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_OWNER)->Get();

		// I must be the weapon's owner
		if (bot->GetEntity() != owner)
		{
			return;
		}

		if (!weapon->GetWeaponInfo()->HasInfiniteAmmo())
		{
			int clip1 = entprops->GetCachedData<int>(pWeapon, CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_CLIP1);
			int primary_ammo_index = entprops->GetCachedData<int>(pWeapon, CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_PRIMARYAMMOTYPE);

			// Must have ammo
			if (clip1 == 0 && bot->GetAmmoOfIndex(primary_ammo_index) == 0)
			{
				return;
			}
		}

		int currentprio = weapon->GetWeaponInfo()->GetPriority();

		if (currentprio > priority)
		{
			best = weapon;
			priority = currentprio;
		}
	});

	m_weaponSwitchCooldown.Start(base_weapon_switch_cooldown());

	if (best != nullptr)
	{
		if (bot->GetBehaviorInterface()->ShouldSwitchToWeapon(bot, best) != ANSWER_NO)
		{
			bot->SelectWeapon(best->GetEntity());
		}
	}
}

void IInventory::SelectBestWeaponForBreakables()
{
	if (m_weaponSwitchCooldown.HasStarted() && !m_weaponSwitchCooldown.IsElapsed())
	{
		return;
	}

	if (m_weapons.empty())
	{
		BuildInventory();
		m_weaponSwitchCooldown.Start(0.250f);
		return;
	}

	CBaseBot* bot = GetBot();
	const CBotWeapon* best = nullptr;
	int priority = std::numeric_limits<int>::min();
	bool found_melee = false;

	// Find the best melee weapon
	ForEveryWeapon([&bot, &priority, &best, &found_melee](const CBotWeapon* weapon) {

		if (!weapon->IsValid())
		{
			return;
		}

		// weapon must be usable against enemies
		if (!weapon->GetWeaponInfo()->IsCombatWeapon())
		{
			return;
		}

		CBaseEntity* pWeapon = weapon->GetEntity();
		CBaseEntity* owner = entprops->GetCachedDataPtr<CHandle<CBaseEntity>>(pWeapon, CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_OWNER)->Get();

		// I must be the weapon's owner
		if (bot->GetEntity() != owner)
		{
			return;
		}

		if (!weapon->GetWeaponInfo()->HasInfiniteAmmo())
		{
			int clip1 = entprops->GetCachedData<int>(pWeapon, CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_CLIP1);
			int primary_ammo_index = entprops->GetCachedData<int>(pWeapon, CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_PRIMARYAMMOTYPE);

			// Must have ammo
			if (clip1 == 0 && bot->GetAmmoOfIndex(primary_ammo_index) == 0)
			{
				return;
			}
		}

		int currentprio = weapon->GetWeaponInfo()->GetPriority();
		bool is_melee = weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).IsMelee();

		if (!is_melee && found_melee)
		{
			return; // if a melee was found, only test for priority if the other weapon is also a melee weapon.
		}

		if (currentprio > priority || (!found_melee && is_melee))
		{
			best = weapon;
			priority = currentprio;

			if (is_melee)
			{
				found_melee = true;
			}
		}
	});

	m_weaponSwitchCooldown.Start(base_weapon_switch_cooldown());

	if (best != nullptr)
	{
		if (bot->GetBehaviorInterface()->ShouldSwitchToWeapon(bot, best) != ANSWER_NO)
		{
			bot->SelectWeapon(best->GetEntity());
		}
	}
}

void IInventory::SelectBestHitscanWeapon(const bool meleeIsHitscan)
{
	if (m_weaponSwitchCooldown.HasStarted() && !m_weaponSwitchCooldown.IsElapsed())
	{
		return;
	}

	if (m_weapons.empty())
	{
		BuildInventory();
		m_weaponSwitchCooldown.Start(0.250f);
		return;
	}

	CBaseBot* bot = GetBot();
	const CBotWeapon* best = nullptr;
	int priority = std::numeric_limits<int>::min();

	// Find the best melee weapon
	ForEveryWeapon([&bot, &priority, &best, &meleeIsHitscan](const CBotWeapon* weapon) {

		if (!weapon->IsValid())
		{
			return;
		}

		// weapon must be usable against enemies
		if (!weapon->GetWeaponInfo()->IsCombatWeapon())
		{
			return;
		}

		if (!weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).IsHitscan())
		{
			return;
		}

		CBaseEntity* pWeapon = weapon->GetEntity();
		CBaseEntity* owner = entprops->GetCachedDataPtr<CHandle<CBaseEntity>>(pWeapon, CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_OWNER)->Get();

		// I must be the weapon's owner
		if (bot->GetEntity() != owner)
		{
			return;
		}

		if (!weapon->GetWeaponInfo()->HasInfiniteAmmo())
		{
			int clip1 = entprops->GetCachedData<int>(pWeapon, CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_CLIP1);
			int primary_ammo_index = entprops->GetCachedData<int>(pWeapon, CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_PRIMARYAMMOTYPE);

			// Must have ammo
			if (clip1 == 0 && bot->GetAmmoOfIndex(primary_ammo_index) == 0)
			{
				return;
			}
		}

		int currentprio = weapon->GetWeaponInfo()->GetPriority();
		bool is_melee = weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).IsMelee();

		if (is_melee && !meleeIsHitscan)
		{
			return;
		}

		if (currentprio > priority)
		{
			best = weapon;
			priority = currentprio;
		}
	});

	m_weaponSwitchCooldown.Start(base_weapon_switch_cooldown());

	if (best != nullptr)
	{
		if (bot->GetBehaviorInterface()->ShouldSwitchToWeapon(bot, best) != ANSWER_NO)
		{
			bot->SelectWeapon(best->GetEntity());
		}
	}
}

std::shared_ptr<CBotWeapon> IInventory::GetActiveBotWeapon()
{
	CBaseEntity* weapon = GetBot()->GetActiveWeapon();

	if (weapon == nullptr)
	{
		return nullptr;
	}

	if (m_cachedActiveWeapon && m_cachedActiveWeapon->IsValid())
	{
		if (m_cachedActiveWeapon->GetEntity() == weapon)
		{
			return m_cachedActiveWeapon;
		}
	}

	for (auto& weaponptr : m_weapons)
	{
		if (weaponptr->IsValid() && weaponptr->GetEntity() == weapon)
		{
			return weaponptr;
		}
	}

	return nullptr;
}

bool IInventory::IsAmmoLow(const bool heldOnly)
{
	CBaseBot* me = GetBot();

	if (heldOnly)
	{
		auto weapon = GetActiveBotWeapon();

		if (!weapon || !weapon->IsValid())
		{
			return false;
		}

		auto& bcw = weapon->GetBaseCombatWeapon();

		int primaryammotype = bcw.GetPrimaryAmmoType();
		int secondaryammotype = bcw.GetSecondaryAmmoType();
		int lowprimary = weapon->GetWeaponInfo()->GetLowPrimaryAmmoThreshold();
		int lowsecondary = weapon->GetWeaponInfo()->GetLowSecondaryAmmoThreshold();

		if (primaryammotype > 0)
		{
			if (me->GetAmmoOfIndex(primaryammotype) <= lowprimary)
			{
				return true;
			}
		}

		if (secondaryammotype > 0)
		{
			if (me->GetAmmoOfIndex(secondaryammotype) <= lowsecondary)
			{
				return true;
			}
		}
	}
	else
	{
		for (auto& weapon : m_weapons)
		{
			if (!weapon->IsValid())
			{
				continue;
			}

			auto& bcw = weapon->GetBaseCombatWeapon();

			int primaryammotype = bcw.GetPrimaryAmmoType();
			int secondaryammotype = bcw.GetSecondaryAmmoType();
			int lowprimary = weapon->GetWeaponInfo()->GetLowPrimaryAmmoThreshold();
			int lowsecondary = weapon->GetWeaponInfo()->GetLowSecondaryAmmoThreshold();

			if (primaryammotype > 0)
			{
				if (me->GetAmmoOfIndex(primaryammotype) <= lowprimary)
				{
					return true;
				}
			}

			if (secondaryammotype > 0)
			{
				if (me->GetAmmoOfIndex(secondaryammotype) <= lowsecondary)
				{
					return true;
				}
			}
		}
	}

	return false;
}

void IInventory::OnWeaponEquip(CBaseEntity* weapon)
{
	this->AddWeaponToInventory(weapon);
}

int IInventory::GetOwnedWeaponCount() const
{
	int count = 0;

	for (auto& weapon : m_weapons)
	{
		if (weapon->IsValid())
		{
			count++;
		}
	}

	return count;
}
