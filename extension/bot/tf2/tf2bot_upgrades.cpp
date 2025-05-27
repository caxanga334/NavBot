#include <cstring>

#include <extension.h>
#include <tier1/KeyValues.h>
#include <mods/tf2/teamfortress2_shareddefs.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/mvm_upgrade_manager.h>
#include <mods/tf2/tf2lib.h>
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
	m_state = STATE_INIT;
}

CTF2BotUpgradeManager::~CTF2BotUpgradeManager()
{
}

void CTF2BotUpgradeManager::Update()
{
	switch (m_state)
	{
	case CTF2BotUpgradeManager::STATE_INVALID:
		m_state = STATE_INIT;
		break;
	case CTF2BotUpgradeManager::STATE_RESET:
		Reset(); // this set the state to init
		break;
	case CTF2BotUpgradeManager::STATE_INIT:
		InitUpgrades();
		break;
	case CTF2BotUpgradeManager::STATE_UPDATEFILTER:
	{
		ExecuteState_UpdateFilter();
		break;
	}
	case CTF2BotUpgradeManager::STATE_REFUND:
		ExecuteState_Refund();
		break;
	case CTF2BotUpgradeManager::STATE_UPGRADE:
		ExecuteState_Upgrade();
		break;
	case CTF2BotUpgradeManager::STATE_ADVANCE:
		ExecuteState_Advance();
		break;
	case CTF2BotUpgradeManager::STATE_DONE:
		return;
		break;
	default:
		break;
	}


}

void CTF2BotUpgradeManager::ExecuteBuy()
{
	if (!m_me->IsInUpgradeZone())
	{
		if (sm_navbot_tf_debug_bot_upgrades.GetBool())
		{
			Vector origin = m_me->GetAbsOrigin();
			smutils->LogError(myself, "BOT %s: CTF2BotUpgradeManager::ExecuteBuy called outsize an upgrade zone at <%3.2f, %3.2f, %3.2f> \n", m_me->GetDebugIdentifier(),
				origin.x, origin.y, origin.z);
		}

		return;
	}

	if (m_tobuylist.empty())
	{
		if (sm_navbot_tf_debug_bot_upgrades.GetBool())
		{
			ConColorMsg(Color(0, 128, 0, 255), "%s: CTF2BotUpgradeManager::ExecuteBuy - Buy list is empty! \n", m_me->GetDebugIdentifier());
			ConColorMsg(Color(0, 128, 128, 255), "Advancing from priority %i to %i!\n", m_nextpriority, (m_nextpriority + 1));
		}

		m_state = STATE_ADVANCE;
		return;
	}

	auto upgradeinfo = SelectRandomUpgradeToBuy();

	if (upgradeinfo == nullptr)
	{
		// SelectRandomUpgradeToBuy will filter upgrades that the bot cannot buy due to lack of currency
		// the list is not empty because that's checked before getting here
		// so the most probable cause is lack of currency to buy any upgrade, in this case, just mark as done for the current wave

		OnDoneUpgrading();
		return;
	}

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

// Selects a random upgrade that the bot can afford, NULL if no upgrade is available
const TF2BotUpgradeInfo_t* CTF2BotUpgradeManager::SelectRandomUpgradeToBuy() const
{
	std::vector<const TF2BotUpgradeInfo_t*> out;
	const int currency = m_me->GetCurrency();

	for (auto info : m_tobuylist)
	{
		if (info->upgrade->CanAfford(currency))
		{
			out.push_back(info);
		}
	}

	if (out.empty())
	{
		return nullptr;
	}

	return out[randomgen->GetRandomInt<size_t>(0, out.size() - 1)];
}

void CTF2BotUpgradeManager::ExecuteState_Refund()
{
	if (!m_me->IsInUpgradeZone())
	{
		if (sm_navbot_tf_debug_bot_upgrades.GetBool())
		{
			Vector origin = m_me->GetAbsOrigin();
			smutils->LogError(myself, "BOT %s: CTF2BotUpgradeManager::ExecuteState_Refund called outsize an upgrade zone at <%3.2f, %3.2f, %3.2f>", m_me->GetDebugIdentifier(),
				origin.x, origin.y, origin.z);
		}

		return;
	}

	ExecuteRefund();
	m_state = STATE_INIT;
}

void CTF2BotUpgradeManager::ExecuteState_Upgrade()
{
	if (AnyUpgradeAvailable())
	{
		// Bot did not buy all available upgrades

		if (!m_tobuylist.empty() && !CanAffordAnyUpgrade())
		{
			OnDoneUpgrading(); // List is not empty, not enough currency to but anything, done for the current wave
			return;
		}
	}
	else
	{
		OnDoneUpgrading();
		return;
	}

	ExecuteBuy();
}

void CTF2BotUpgradeManager::ExecuteState_Advance()
{
	AdvancePriority();
	FetchUpgrades();
	m_state = STATE_UPGRADE;
}

void CTF2BotUpgradeManager::ExecuteState_UpdateFilter()
{
	if (sm_navbot_tf_debug_bot_upgrades.GetBool())
	{
		ConColorMsg(Color(0, 200, 200, 255), "%s: Running Upgrade Filter!\n", m_me->GetDebugIdentifier());
	}

	FilterUpgrades();
	m_state = STATE_UPGRADE;
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

	if (sm_navbot_tf_debug_bot_upgrades.GetBool())
	{
		ConColorMsg(Color(0, 128, 0, 255), "%s: Collected %i upgrades for class %s. Current Priority: %i\n", m_me->GetDebugIdentifier(), static_cast<int>(m_tobuylist.size()), tf2lib::GetClassNameFromType(m_me->GetMyClassType()), m_nextpriority);
	}

	FilterUpgrades();
}

void CTF2BotUpgradeManager::FilterUpgrades()
{
	if (m_me->GetInventoryInterface()->HasAnyWeapons())
	{
		m_state = STATE_UPDATEFILTER;
		m_me->GetInventoryInterface()->BuildInventory();
		return;
	}

	// Collect weapon item definition indexes
	std::vector<int> myweaponindexes;
	auto functor = [&myweaponindexes](const CBotWeapon* weapon) {
		myweaponindexes.push_back(weapon->GetWeaponEconIndex());
	};

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

	m_me->GetInventoryInterface()->ForEveryWeapon(functor);

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

const bool CTF2BotUpgradeManager::HasBoughtUpgrade(const char* upgrade) const
{
	for (auto& data : m_boughtlist)
	{
		if (std::strcmp(upgrade, data.upgrade->attribute.c_str()) == 0)
		{
			return true;
		}
	}

	return false;
}

