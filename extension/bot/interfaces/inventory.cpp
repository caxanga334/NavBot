#include <limits>
#include <extension.h>
#include <util/entprops.h>
#include <bot/basebot.h>
#include "behavior.h"
#include "knownentity.h"
#include "inventory.h"

// undef some stuff from valve's mathlib that conflicts with STL
#undef min
#undef max
#undef clamp

constexpr auto UPDATE_WEAPONS_INTERVAL = 15.0f;
constexpr auto UPDATE_WEAPONS_INTERVAL_AFTER_RESET = 0.5f;

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
}

void IInventory::Update()
{
	if (m_updateWeaponsTimer.IsElapsed())
	{
		m_updateWeaponsTimer.Start(UPDATE_WEAPONS_INTERVAL);
		BuildInventory();
	}
}

void IInventory::Frame()
{
}

bool IInventory::HasWeapon(std::string classname)
{
	for (auto& weapon : m_weapons)
	{
		if (!weapon->IsValid())
			continue;

		const char* clname = weapon->GetBaseCombatWeapon().GetClassname();

		if (clname != nullptr)
		{
			std::string szName(clname);

			if (szName == classname)
			{
				return true;
			}
		}
	}

	return false;
}

void IInventory::BuildInventory()
{
	m_weapons.clear();

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		int index = INVALID_EHANDLE_INDEX;
		entprops->GetEntPropEnt(GetBot()->GetIndex(), Prop_Send, "m_hMyWeapons", index, i);

		if (index == INVALID_EHANDLE_INDEX)
			continue;

		edict_t* weapon = gamehelpers->EdictOfIndex(index);

		if (!UtilHelpers::IsValidEdict(weapon))
			continue;

		// IsValidEdict checks if GetIServerEntity return is not NULL
		m_weapons.emplace_back(new CBotWeapon(weapon->GetIServerEntity()->GetBaseEntity()));
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
		
		// weapon must be usable against enemies
		if (!weapon->GetWeaponInfo()->IsCombatWeapon())
		{
			return;
		}

		// Must have ammo
		if (weapon->GetBaseCombatWeapon().GetClip1() == 0 && bot->GetAmmoOfIndex(weapon->GetBaseCombatWeapon().GetPrimaryAmmoType()) == 0)
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
	edict_t* weapon = GetBot()->GetActiveWeapon();

	if (!UtilHelpers::IsValidEdict(weapon))
	{
		return nullptr;
	}

	if (m_cachedActiveWeapon)
	{
		if (m_cachedActiveWeapon->GetEdict() == weapon)
		{
			return m_cachedActiveWeapon;
		}
	}

	for (auto& weaponptr : m_weapons)
	{
		if (weaponptr->GetEdict() == weapon)
		{
			return weaponptr;
		}
	}

	return nullptr;
}
