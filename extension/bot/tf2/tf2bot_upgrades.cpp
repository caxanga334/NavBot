#include <extension.h>
#include <tier1/KeyValues.h>
#include <mods/tf2/teamfortress2_shareddefs.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/mvm_upgrade_manager.h>
#include <util/librandom.h>
#include <util/helpers.h>
#include "tf2bot.h"
#include "tf2bot_upgrades.h"

static ConVar sm_navbot_tf_debug_bot_upgrades("sm_navbot_tf_debug_bot_upgrades", "0", FCVAR_CHEAT, "Enables debugging bot upgrades.");

CTF2BotUpgradeManager::CTF2BotUpgradeManager()
{
	m_me = nullptr;
	m_nextpriority = 1;
	m_numupgrades = 0;
	m_tobuylist.reserve(16);
	m_boughtlist.reserve(32);
	m_state = STATE_GOBUY;
}

CTF2BotUpgradeManager::~CTF2BotUpgradeManager()
{
}

void CTF2BotUpgradeManager::Update()
{
	switch (m_state)
	{
	case CTF2BotUpgradeManager::STATE_GOBUY:

		// If buy list is empty, advance into the next priority
		if (m_tobuylist.empty())
		{
			if (!AnyUpgradeAvailable())
			{
				OnDoneUpgrading();
				return;
			}

			if (sm_navbot_tf_debug_bot_upgrades.GetBool())
			{
				ConColorMsg(Color(0, 128, 0, 255), "%s: Buy list is empty!\n", m_me->GetDebugIdentifier());
				ConColorMsg(Color(0, 128, 128, 255), "Advancing from priority %i to %i!\n", m_nextpriority, (m_nextpriority + 1));
			}

			m_state = STATE_ADVANCE;
			return;
		}

		// I have stuff to buy but I can't afford anything
		if (!CanAffordAnyUpgrade())
		{
			OnDoneUpgrading();
			return;
		}

		m_state = STATE_BUYINGSTUFF; // enter buy state
		return;
	case CTF2BotUpgradeManager::STATE_BUYINGSTUFF:
		ExecuteBuy();
		RemoveFinishedUpgrades(); // clean up

		if (!CanAffordAnyUpgrade())
		{
			OnDoneUpgrading();
			return;
		}
		return;
	case CTF2BotUpgradeManager::STATE_ADVANCE:
		AdvancePriority();
		FetchUpgrades();
		m_state = STATE_GOBUY;
		return;
	case CTF2BotUpgradeManager::STATE_WAITFORNEXTWAVE:
		return;
	case CTF2BotUpgradeManager::STATE_DOREFUND:
		ExecuteRefund();
		OnUpgradesRefunded();
		return;
	case CTF2BotUpgradeManager::STATE_REFILTER:
		if (sm_navbot_tf_debug_bot_upgrades.GetBool())
		{
			ConColorMsg(Color(0, 200, 200, 255), "%s: Running Upgrade Filter!\n", m_me->GetDebugIdentifier());
		}

		FilterUpgrades();
		m_state = STATE_GOBUY;
		return;
	default:
		return;
	}
}

void CTF2BotUpgradeManager::ExecuteBuy()
{
	if (!m_me->IsInUpgradeZone())
	{
		if (sm_navbot_tf_debug_bot_upgrades.GetBool())
		{
			Vector origin = m_me->GetAbsOrigin();
			smutils->LogError(myself, "BOT %s: CTF2BotUpgradeManager::Update called outsize an upgrade zone at <%3.2f, %3.2f, %3.2f>", m_me->GetDebugIdentifier(),
				origin.x, origin.y, origin.z);
		}

		return;
	}

	if (m_tobuylist.empty())
	{
		if (sm_navbot_tf_debug_bot_upgrades.GetBool())
		{
			ConColorMsg(Color(0, 128, 0, 255), "%s: CTF2BotUpgradeManager::Update - Buy list is empty!", m_me->GetDebugIdentifier());
			ConColorMsg(Color(0, 128, 128, 255), "Advancing from priority %i to %i!\n", m_nextpriority, (m_nextpriority + 1));
		}

		m_state = STATE_ADVANCE;
		return;
	}

	auto upgradeinfo = m_tobuylist[randomgen->GetRandomInt<size_t>(0U, m_tobuylist.size() - 1U)];
	int currency = m_me->GetCurrency();
	auto data = GetOrCreateUpgradeData(upgradeinfo->upgrade);

	// Upgrade reached desired level limit, remove it from the to-buy list
	if (upgradeinfo->HasLevelLimit() && data->times_bought >= upgradeinfo->GetLevelLimit())
	{
		RemoveSingleUpgradeInfo(upgradeinfo);
		return;
	}

	// Upgrade reached the max in game level, remove it from the list
	if (data->IsMaxed())
	{
		RemoveFinishedUpgrades(); // clean up
		return;
	}

	// Can't afford it
	if (!upgradeinfo->upgrade->CanAfford(m_me->GetCurrency()))
	{
		return;
	}

	if (sm_navbot_tf_debug_bot_upgrades.GetBool())
	{
		ConColorMsg(Color(32, 255, 0, 255), "%s: Bought Upgrade -->\n", m_me->GetDebugIdentifier());
		ConColorMsg(Color(0, 255, 255, 255), "    ID: %i\n    Attribute: %s\n    Quality: %i\n    Item Slot: %i\n    Times Bought: %i\n", 
			upgradeinfo->GetUpgradeIndex(), upgradeinfo->attribute.c_str(), upgradeinfo->quality, upgradeinfo->itemslot, data->times_bought);
	}

	BeginBuyingUpgrades();
	BuySingleUpgrade(upgradeinfo->GetUpgradeIndex(), upgradeinfo->itemslot, 1);
	EndBuyingUpgrades();
	data->times_bought++; // for now we must assume that buying an upgrade will never fail
}

void CTF2BotUpgradeManager::ExecuteRefund()
{
	BeginBuyingUpgrades();
	RefundUpgrades();
	EndBuyingUpgrades();
}

bool CTF2BotUpgradeManager::CanAffordAnyUpgrade() const
{
	int currency = m_me->GetCurrency();

	for (auto data : m_tobuylist)
	{
		if (data->upgrade->CanAfford(currency))
		{
			return true;
		}
	}

	return false;
}

bool CTF2BotUpgradeManager::AnyUpgradeAvailable() const
{
	const int maxprio = CTeamFortress2Mod::GetTF2Mod()->GetMvMUpgradeManager().GetMaxPriorityForClass(m_me->GetMyClassType());

	if (m_nextpriority <= maxprio)
	{
		return true;
	}

	if (sm_navbot_tf_debug_bot_upgrades.GetBool())
	{
		ConColorMsg(Color(128, 255, 0, 255), "%s reached upgrade priority limit of %i (%i)!\n", m_me->GetDebugIdentifier(), maxprio, m_nextpriority);
	}

	return false;
}

void CTF2BotUpgradeManager::BeginBuyingUpgrades()
{
	KeyValues* kvcmd = new KeyValues("MvM_UpgradesBegin");
	UtilHelpers::FakeClientCommandKeyValues(m_me->GetEdict(), kvcmd);
	// kvcmd->deleteThis(); // KeyValues sent as commands are deleted by the engine
	// kvcmd = nullptr;
}

void CTF2BotUpgradeManager::BuySingleUpgrade(const int upgradeID, const int itemslot, const int quantity)
{
	KeyValues* kvcmd = new KeyValues("MVM_Upgrade");
	KeyValues* kUpg = kvcmd->FindKey("Upgrade", true);
	kUpg->SetInt("itemslot", itemslot);
	kUpg->SetInt("Upgrade", upgradeID);
	kUpg->SetInt("count", quantity);
	UtilHelpers::FakeClientCommandKeyValues(m_me->GetEdict(), kvcmd);
	// kvcmd->deleteThis();
	// kvcmd = nullptr;
	m_numupgrades += quantity;
}

void CTF2BotUpgradeManager::RefundUpgrades()
{
	KeyValues* kvcmd = new KeyValues("MVM_Respec");
	UtilHelpers::FakeClientCommandKeyValues(m_me->GetEdict(), kvcmd);
	// kvcmd->deleteThis();
	// kvcmd = nullptr;
	m_numupgrades = 0;
}

void CTF2BotUpgradeManager::EndBuyingUpgrades()
{
	KeyValues* kvcmd = new KeyValues("MvM_UpgradesDone");
	kvcmd->SetInt("num_upgrades", m_numupgrades);
	UtilHelpers::FakeClientCommandKeyValues(m_me->GetEdict(), kvcmd);
	// kvcmd->deleteThis();
	// kvcmd = nullptr;
	m_numupgrades = 0;
}

void CTF2BotUpgradeManager::FetchUpgrades()
{
	CTeamFortress2Mod::GetTF2Mod()->GetMvMUpgradeManager().CollectUpgradesToBuy(m_tobuylist, m_nextpriority, m_me->GetMyClassType());
	FilterUpgrades();
}

void CTF2BotUpgradeManager::FilterUpgrades()
{
	if (m_me->GetMyWeaponsCount() == 0)
	{
		m_state = STATE_REFILTER;
		m_me->UpdateMyWeapons(); // Force an update
		return;
	}


	// Collect weapon item definition indexes
	std::vector<int> myweaponindexes;
	m_me->ForEveryWeapon([&myweaponindexes](const CBotWeapon& weapon) {
		myweaponindexes.push_back(weapon.GetWeaponEconIndex());
	});

	auto start = std::remove_if(m_tobuylist.begin(), m_tobuylist.end(), [&myweaponindexes](const TF2BotUpgradeInfo_t* upgradeinfo) {
		if (upgradeinfo->AnyWeapon())
		{
			return false; // no restrictions on weapons
		}

		for (auto& index : myweaponindexes)
		{
			if (upgradeinfo->allowedweapons.find(index) != upgradeinfo->allowedweapons.end())
			{
				return false; // bot has a weapon that can use this upgrade
			}
		}

		return true; // upgrade has weapon restriction and the bot doesn't have the needed weapon, remove this upgrade
	});

	if (sm_navbot_tf_debug_bot_upgrades.GetBool())
	{
		for (auto it = start; it != m_tobuylist.end(); it++)
		{
			auto upgrade = *it;

			ConColorMsg(Color(255, 0, 0, 255), "%s: Removing invalid upgrade %i <%s, %i, %i>. (Weapon Restriction)\n", m_me->GetDebugIdentifier(),
				upgrade->GetUpgradeIndex(), upgrade->attribute.c_str(), upgrade->quality, upgrade->itemslot);
		}
	}

	m_tobuylist.erase(start, m_tobuylist.end()); // remove upgrades

	auto start2 = std::remove_if(m_tobuylist.begin(), m_tobuylist.end(), [&myweaponindexes](const TF2BotUpgradeInfo_t* upgradeinfo) {
		if (!upgradeinfo->AreThereExcludedWeapons())
		{
			return false; // no weapon exclusions
		}

		for (auto& index : myweaponindexes)
		{
			if (upgradeinfo->IsWeaponExcluded(index))
			{
				return true; // bot contains a weapon that is excluded for this upgrade
			}
		}

		return false;
	});

	if (sm_navbot_tf_debug_bot_upgrades.GetBool())
	{
		for (auto it = start2; it != m_tobuylist.end(); it++)
		{
			auto upgrade = *it;

			ConColorMsg(Color(255, 0, 0, 255), "%s: Removing invalid upgrade %i <%s, %i, %i>. (Weapon Exclusions)\n", m_me->GetDebugIdentifier(),
				upgrade->GetUpgradeIndex(), upgrade->attribute.c_str(), upgrade->quality, upgrade->itemslot);
		}
	}

	m_tobuylist.erase(start2, m_tobuylist.end()); // remove upgrades
}

void CTF2BotUpgradeManager::RemoveFinishedUpgrades()
{
	auto start = std::remove_if(m_tobuylist.begin(), m_tobuylist.end(), [this](const TF2BotUpgradeInfo_t* upgradeinfo) {
		for (auto& bought : m_boughtlist)
		{
			if (bought.IsMaxed() && bought.upgrade->index == upgradeinfo->upgrade->index)
			{
				return true; // upgrade maxed, remove it
			}
		}

		return false;
	});

	m_tobuylist.erase(start, m_tobuylist.end()); // remove upgrades
}

void CTF2BotUpgradeManager::RemoveSingleUpgradeInfo(const TF2BotUpgradeInfo_t* info)
{
	auto start = std::remove_if(m_tobuylist.begin(), m_tobuylist.end(), [&info](const TF2BotUpgradeInfo_t* object) {
		if (object == info)
		{
			return true;
		}

		return false;
	});

	m_tobuylist.erase(start, m_tobuylist.end()); // remove upgrades
}

TF2BotUpgrade_t* CTF2BotUpgradeManager::GetOrCreateUpgradeData(const MvMUpgrade_t* upgrade)
{
	for (auto& data : m_boughtlist)
	{
		if (data.upgrade->index == upgrade->index)
		{
			return &data;
		}
	}

	auto& newdata = m_boughtlist.emplace_back(upgrade);
	return &newdata;
}

