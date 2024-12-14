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

		int clip1 = entprops->GetCachedData<int>(pWeapon, CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_CLIP1);
		int primary_ammo_index = entprops->GetCachedData<int>(pWeapon, CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_PRIMARYAMMOTYPE);

		// Must have ammo
		if (clip1 == 0 && bot->GetAmmoOfIndex(primary_ammo_index) == 0)
		{
			return;
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
