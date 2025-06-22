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

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

// undef some stuff from valve's mathlib that conflicts with STL
#undef min
#undef max
#undef clamp

constexpr auto UPDATE_WEAPONS_INTERVAL_AFTER_RESET = 0.2f;

IInventory::IInventory(CBaseBot* bot) : IBotInterface(bot)
{
	m_weapons.reserve(MAX_WEAPONS);
	m_updateWeaponsTimer.Start(UPDATE_WEAPONS_INTERVAL_AFTER_RESET);
	m_cachedActiveWeapon = nullptr;
	m_purgeStaleWeaponsTimer.Start(2.0f);
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
	m_purgeStaleWeaponsTimer.Start(2.0f);
}

void IInventory::Update()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IInventory::Update", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (m_updateWeaponsTimer.IsElapsed())
	{
		m_updateWeaponsTimer.Start(extmanager->GetMod()->GetModSettings()->GetInventoryUpdateRate());
		BuildInventory();
	}

	if (m_purgeStaleWeaponsTimer.IsElapsed())
	{
		m_purgeStaleWeaponsTimer.Start(2.0f);
		const CBaseBot* me = GetBot<CBaseBot>();

		m_weapons.erase(std::remove_if(m_weapons.begin(), m_weapons.end(), [&me](const std::unique_ptr<CBotWeapon>& object) {

			CBaseEntity* pWeapon = object->GetEntity();

			if (!pWeapon)
			{
				return true; // NULL weapon entity
			}
			
			CBaseEntity* owner = entprops->GetCachedDataPtr<CHandle<CBaseEntity>>(pWeapon, CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_OWNER)->Get();

			if (owner != me->GetEntity())
			{
				return true; // I no longer own this weapon
			}

			return false;

		}), m_weapons.end());
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
	return std::any_of(std::begin(m_weapons), std::end(m_weapons), [&classname](const std::unique_ptr<CBotWeapon>& weaponptr) {
		return weaponptr->IsValid() && (strcmp(weaponptr->GetClassname().c_str(), classname) == 0);
	});
}

bool IInventory::HasWeapon(CBaseEntity* weapon)
{
	return std::any_of(std::begin(m_weapons), std::end(m_weapons), [&weapon](const std::unique_ptr<CBotWeapon>& weaponptr) {
		return weaponptr->GetEntity() == weapon;
	});
}

CBaseEntity* IInventory::GetWeaponEntity(const char* classname)
{
	auto it = std::find_if(std::begin(m_weapons), std::end(m_weapons), [&classname](const std::unique_ptr<CBotWeapon>& weaponptr) {
		return weaponptr->IsValid() && (strcmp(weaponptr->GetClassname().c_str(), classname) == 0);
	});

	if (it != m_weapons.end())
	{
		return it->get()->GetEntity();
	}

	return nullptr;
}

const CBotWeapon* IInventory::GetWeaponOfEntity(CBaseEntity* weapon)
{
	auto it = std::find_if(std::begin(m_weapons), std::end(m_weapons), [&weapon](const std::unique_ptr<CBotWeapon>& weaponptr) {
		return weaponptr->GetEntity() == weapon;
	});

	if (it != m_weapons.end())
	{
		return it->get();
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
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IInventory::BuildInventory", "NavBot");
#endif // EXT_VPROF_ENABLED

	m_weapons.clear();
	m_cachedActiveWeapon = nullptr;

	// array CHandle<CBaseEntity> weapons[MAX_WEAPONS]
	CHandle<CBaseEntity>* myweapons = entprops->GetCachedDataPtr<CHandle<CBaseEntity>>(GetBot()->GetEntity(), CEntPropUtils::CacheIndex::CBASECOMBATCHARACTER_MYWEAPONS);

#ifdef EXT_DEBUG
	{
		static bool logged = false;

		if (!logged)
		{
			logged = true;

			std::size_t size = entprops->GetEntPropArraySize(GetBot<CBaseBot>()->GetIndex(), Prop_Send, "m_hMyWeapons");

			if (size != static_cast<std::size_t>(MAX_WEAPONS))
			{
				smutils->LogError(myself, "SDK's MAX_WEAPONS macro (%i) doesn't match CBaseCombatCharacter::m_hMyWeapons array size (%zu)!", MAX_WEAPONS, size);
			}
		}
	}
#endif // EXT_DEBUG

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
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IInventory::SelectBestWeaponForThreat", "NavBot");
#endif // EXT_VPROF_ENABLED

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
	auto func = [&bot, &priority, &rangeToThreat, &best, &threat](const CBotWeapon* weapon) {

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

		if (weapon->IsOutOfAmmo(bot))
		{
			return;
		}

		if (!weapon->GetWeaponInfo()->IsInSelectionRange(rangeToThreat))
		{
			return;
		}

		int currentprio = weapon->GetPriority(bot, &rangeToThreat, threat);

		if (currentprio > priority)
		{
			best = weapon;
			priority = currentprio;
		}
	};

	ForEveryWeapon(func);

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
	auto func = [&bot, &priority, &best](const CBotWeapon* weapon) {

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
#ifdef EXT_DEBUG
			META_CONPRINTF("BOT WEAPON OWNER MISMATCH FOR WEAPON %s #%i!\n    %p != %p \n", 
				weapon->GetClassname().c_str(), weapon->GetIndex(), bot->GetEntity(), owner);
#endif // EXT_DEBUG
			return;
		}

		if (weapon->IsOutOfAmmo(bot))
		{
			return;
		}

		int currentprio = weapon->GetPriority(bot);

		if (currentprio > priority)
		{
			best = weapon;
			priority = currentprio;
		}
	};

	ForEveryWeapon(func);

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
	auto func = [&bot, &priority, &best, &found_melee](const CBotWeapon* weapon) {

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

		if (weapon->IsOutOfAmmo(bot))
		{
			return;
		}

		int currentprio = weapon->GetPriority(bot);
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
	};

	// Find the best melee weapon
	ForEveryWeapon(func);

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
	auto func = [&bot, &priority, &best, &meleeIsHitscan](const CBotWeapon* weapon) {

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

		if (weapon->IsOutOfAmmo(bot))
		{
			return;
		}

		int currentprio = weapon->GetPriority(bot);
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
	};

	// Find the best melee weapon
	ForEveryWeapon(func);

	m_weaponSwitchCooldown.Start(base_weapon_switch_cooldown());

	if (best != nullptr)
	{
		if (bot->GetBehaviorInterface()->ShouldSwitchToWeapon(bot, best) != ANSWER_NO)
		{
			bot->SelectWeapon(best->GetEntity());
		}
	}
}

const CBotWeapon* IInventory::GetActiveBotWeapon() const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IInventory::GetActiveBotWeapon", "NavBot");
#endif // EXT_VPROF_ENABLED

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
			m_cachedActiveWeapon = weaponptr.get();
			return weaponptr.get();
		}
	}

	return nullptr;
}

bool IInventory::IsAmmoLow(const bool heldOnly)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IInventory::IsAmmoLow", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseBot* me = GetBot();

	if (heldOnly)
	{
		const CBotWeapon* weapon = GetActiveBotWeapon();

		if (!weapon || !weapon->IsValid())
		{
			return false;
		}

		return weapon->IsAmmoLow(me);
	}
	else
	{
		for (auto& weapon : m_weapons)
		{
			if (!weapon->IsValid())
			{
				continue;
			}

			if (weapon->IsAmmoLow(me))
			{
				return true;
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
