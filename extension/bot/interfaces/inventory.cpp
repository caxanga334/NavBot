#include NAVBOT_PCH_FILE
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
	// Building the inventory at reset is fine for vanilla since the bot already has their weapons
	// Might have issues with weapons given post spawn
	BuildInventory();
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
		m_purgeStaleWeaponsTimer.Start(0.5f);
		RemoveInvalidWeapons();
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

const CBotWeapon* IInventory::FindWeaponByClassnamePattern(const char* pattern, const bool validateOwnership) const
{
	for (auto& weaponptr : m_weapons)
	{
		if (weaponptr->IsValid() && UtilHelpers::StringMatchesPattern(weaponptr->GetClassname().c_str(), pattern, 0))
		{
			if (validateOwnership && !weaponptr->IsOwnedByBot(GetBot<CBaseBot>()))
			{
				continue;
			}

			return weaponptr.get();
		}
	}

	return nullptr;
}

bool IInventory::EquipWeapon(const CBotWeapon* weapon) const
{
	const CBotWeapon* activeWeapon = GetActiveBotWeapon();

	if (activeWeapon != nullptr && activeWeapon == weapon)
	{
		return false;
	}

	CBaseBot* me = GetBot<CBaseBot>();

	me->SelectWeapon(weapon->GetEntity());
	OnBotWeaponEquipped(weapon);

	return true;
}

bool IInventory::EquipWeapon(CBaseEntity* weapon) const
{
	CBaseBot* me = GetBot<CBaseBot>();

	CBaseEntity* active = me->GetActiveWeapon();

	if (active == weapon)
	{
		return false;
	}

	me->SelectWeapon(weapon);
	const CBotWeapon* botWeapon = GetWeaponOfEntity(weapon);

	if (botWeapon)
	{
		OnBotWeaponEquipped(botWeapon);
	}

	return true;
}

bool IInventory::HasWeapon(const char* classname) const
{
	return std::any_of(std::begin(m_weapons), std::end(m_weapons), [&classname](const std::unique_ptr<CBotWeapon>& weaponptr) {
		return weaponptr->IsValid() && (strcmp(weaponptr->GetClassname().c_str(), classname) == 0);
	});
}

bool IInventory::HasWeapon(CBaseEntity* weapon) const
{
	return std::any_of(std::begin(m_weapons), std::end(m_weapons), [&weapon](const std::unique_ptr<CBotWeapon>& weaponptr) {
		return weaponptr->GetEntity() == weapon;
	});
}

CBaseEntity* IInventory::GetWeaponEntity(const char* classname) const
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

const CBotWeapon* IInventory::GetWeaponOfEntity(CBaseEntity* weapon) const
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

const CBotWeapon* IInventory::AddWeaponToInventory(CBaseEntity* weapon)
{
	if (!HasWeapon(weapon))
	{
		auto& instance =  m_weapons.emplace_back(CreateBotWeapon(weapon));
		return instance.get();
	}

	return nullptr;
}

const CBotWeapon* IInventory::GetOrCreateWeaponOfEntity(CBaseEntity* weapon)
{
	CBaseEntity* owner = nullptr;
	CBaseBot* me = GetBot<CBaseBot>();

	entprops->GetEntPropEnt(weapon, Prop_Send, "m_hOwner", nullptr, &owner);

	if (owner != me->GetEntity())
	{
		return nullptr;
	}

	const CBotWeapon* instance = GetWeaponOfEntity(weapon);

	if (instance)
	{
		return instance;
	}

	return AddWeaponToInventory(weapon);
}

void IInventory::BuildInventory()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IInventory::BuildInventory", "NavBot");
#endif // EXT_VPROF_ENABLED

	RemoveInvalidWeapons();

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

	// TO-DO, might be better to replace the MAX_WEAPONS macros with a runtime array size from GetEntPropArraySize
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		CBaseEntity* weapon = myweapons[i].Get();

		if (weapon == nullptr)
		{
			continue;
		}

		// AddWeaponToInventory will check for duplicates
		AddWeaponToInventory(weapon);
	}
}

bool IInventory::SelectBestWeaponForThreat(const CKnownEntity* threat, WeaponInfo::WeaponType typeOnly)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("IInventory::SelectBestWeaponForThreat", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (!m_weaponSwitchCooldown.IsElapsed() || m_weapons.empty())
	{
		return false;
	}

	m_weaponSwitchCooldown.Start(base_weapon_switch_cooldown());

	CBaseBot* bot = GetBot();
	const float rangeToThreat = bot->GetRangeTo(UtilHelpers::getWorldSpaceCenter(threat->GetEntity()));
	const CBotWeapon* best = nullptr;
	const DifficultyProfile* profile = bot->GetDifficultyProfile();

	for (auto& weaponptr : m_weapons)
	{
		const CBotWeapon* weapon = weaponptr.get();
		const WeaponInfo* info = weaponptr->GetWeaponInfo();

		if (!IsWeaponUseableForThreat(bot, threat, rangeToThreat, weapon, info))
		{
			continue;
		}

		// Using type filter
		if (typeOnly != WeaponInfo::WeaponType::MAX_WEAPON_TYPES)
		{
			if (info->GetWeaponType() != typeOnly)
			{
				continue;
			}
		}
		else
		{
			// Default filter
			if (!info->IsCombatWeapon())
			{
				continue;
			}
		}

		// Too stupid to use it
		if (info->GetMinRequiredSkill() > profile->GetWeaponSkill())
		{
			continue;
		}

		// assign the first weapon
		if (best == nullptr)
		{
			best = weapon;
			continue;
		}

		// sanity check
		if (best == weapon) { continue; }

		const CBotWeapon* result = FilterBestWeaponForThreat(bot, threat, rangeToThreat, best, weapon);

		if (!result) { return false; }

		best = result;
	}

	if (best != nullptr)
	{
		if (bot->GetBehaviorInterface()->ShouldSwitchToWeapon(bot, best) != ANSWER_NO)
		{
			return EquipWeapon(best);
		}
	}

	return false;
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

void IInventory::SelectBestWeaponForBreakables(const bool force)
{
	if (force) { m_weaponSwitchCooldown.Invalidate(); }

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
			EquipWeapon(best);
		}
	}
}

void IInventory::SelectBestWeaponOfType(WeaponInfo::WeaponType type, const float range)
{
	if (m_weaponSwitchCooldown.HasStarted() && !m_weaponSwitchCooldown.IsElapsed())
	{
		return;
	}

	CBaseBot* bot = GetBot();
	const CBotWeapon* best = nullptr;

	for (auto& ptr : m_weapons)
	{
		const CBotWeapon* weapon = ptr.get();
		const WeaponInfo* info = weapon->GetWeaponInfo();

		if (!weapon->IsValid() || !weapon->IsOwnedByBot(bot))
		{
			continue;
		}

		if (info->GetWeaponType() != type)
		{
			continue;
		}

		if (weapon->IsOutOfAmmo(bot))
		{
			continue;
		}

		if (range > 0.0f)
		{
			if (!info->IsInSelectionRange(range))
			{
				continue;
			}
		}

		if (best == nullptr)
		{
			best = weapon;
			continue;
		}

		if (weapon == best)
		{
			continue;
		}

		best = FilterSelectWeaponWithHighestStaticPriority(best, weapon);
	}

	if (best != nullptr)
	{
		m_weaponSwitchCooldown.Start(base_weapon_switch_cooldown());

		if (bot->GetBehaviorInterface()->ShouldSwitchToWeapon(bot, best) != ANSWER_NO)
		{
			EquipWeapon(best);
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
			EquipWeapon(best);
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

void IInventory::RemoveInvalidWeapons()
{
	CBaseBot* me = GetBot<CBaseBot>();
	m_cachedActiveWeapon = nullptr; // purge the cache just to be safe

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

const CBotWeapon* IInventory::FilterBestWeaponForThreat(CBaseBot* me, const CKnownEntity* threat, const float rangeToThreat, const CBotWeapon* first, const CBotWeapon* second) const
{
	if (first->GetPriority(me, &rangeToThreat, threat) > second->GetPriority(me, &rangeToThreat, threat))
	{
		return first;
	}

	return second;
}

bool IInventory::IsWeaponUseableForThreat(CBaseBot* me, const CKnownEntity* threat, const float rangeToThreat, const CBotWeapon* weapon, const WeaponInfo* info) const
{
	if (!weapon->IsValid() || !weapon->IsOwnedByBot(me) || weapon->IsOutOfAmmo(me) || !info->IsInSelectionRange(rangeToThreat))
	{
		return false;
	}

	if (!weapon->CanUsePrimaryAttack(me) && !weapon->CanUseSecondaryAttack(me))
	{
		return false;
	}

	return true;
}

const CBotWeapon* IInventory::FilterSelectWeaponWithHighestStaticPriority(const CBotWeapon* first, const CBotWeapon* second) const
{
	if (first->GetWeaponInfo()->GetPriority() >= second->GetWeaponInfo()->GetPriority())
	{
		return first;
	}

	return second;
}

void IInventory::RefreshInventory::operator()(CBaseBot* bot)
{
	bot->GetInventoryInterface()->StartInventoryUpdateTimer(m_delay);
}
