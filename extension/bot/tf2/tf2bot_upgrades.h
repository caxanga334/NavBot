#ifndef NAVBOT_TF2_MVM_UPGRADES_H_
#define NAVBOT_TF2_MVM_UPGRADES_H_
#pragma once

#include <vector>
#include <mods/tf2/mvm_upgrade.h>

class CTF2Bot;

class CTF2BotUpgradeManager
{
public:
	CTF2BotUpgradeManager();
	~CTF2BotUpgradeManager();
	static constexpr int INITIAL_PRIORITY = 1;

	void SetMe(CTF2Bot* me) { m_me = me; }

	enum BuyUpgradeState : int
	{
		STATE_INVALID = 0, // Invalid state
		STATE_RESET, // Reset the manager
		STATE_INIT, // Initialize the upgrades
		STATE_UPDATEFILTER, // filter is outdated, update it
		STATE_REFUND, // Refund my upgrades
		STATE_UPGRADE, // Go to an upgrade station and buy stuff
		STATE_ADVANCE, // Advance the upgrade list
		STATE_DONE, // Bot is done upgrading, wait for the next wave

		MAX_STATES
	};

	// Call this to make the bot buy an upgrade
	void Update();

	void Reset()
	{
		m_numupgrades = 0;
		m_nextpriority = 1;
		m_tobuylist.clear();
		m_boughtlist.clear();
		m_state = STATE_INIT;
	}

	void OnUpgradesRefunded()
	{
		m_tobuylist.clear();
		m_numupgrades = 0;
		m_state = STATE_RESET;
	}

	void OnDoneUpgrading()
	{
		m_state = STATE_DONE;
	}

	// Called when a wave ends
	void OnWaveEnd()
	{
		m_state = STATE_UPGRADE;
	}

	// Called when a wave is failed
	void OnWaveFailed()
	{
		// It's hard to keep track of upgrades during wave failures, just refund everything and start again
		m_state = STATE_REFUND;
	}

	void OnBotSpawn()
	{
		if (m_state == STATE_INVALID || m_state == STATE_INIT)
		{
			InitUpgrades();
		}
	}

	TF2BotUpgrade_t* GetOrCreateUpgradeData(const MvMUpgrade_t* upgrade);
	void DoRefund() { m_state = STATE_REFUND; }

	const int GetCurrentPriority() const { return m_nextpriority; }
	const bool IsUpgradingDone() const { return m_tobuylist.empty(); }
	const bool IsAtInitialPriority() const { return m_nextpriority == INITIAL_PRIORITY; }
	const bool ShouldGoToAnUpgradeStation() const { return m_state == STATE_UPGRADE || m_state == STATE_REFUND || m_state == STATE_ADVANCE; }
	const bool IsDoneForCurrentWave() const { return m_state == STATE_DONE; }
	const bool IsManagerReady() const { return m_state >= STATE_REFUND; }
	BuyUpgradeState GetState() const { return m_state; }

private:
	CTF2Bot* m_me;
	int m_numupgrades;
	int m_nextpriority;
	std::vector<const TF2BotUpgradeInfo_t*> m_tobuylist;
	std::vector<TF2BotUpgrade_t> m_boughtlist;
	BuyUpgradeState m_state;

	const TF2BotUpgradeInfo_t* SelectRandomUpgradeToBuy() const;

	void InitUpgrades()
	{
		m_nextpriority = 1;
		m_state = STATE_UPDATEFILTER;
		FetchUpgrades();
	}

	void ExecuteState_Refund();
	void ExecuteState_Upgrade();
	void ExecuteState_Advance();
	void ExecuteState_UpdateFilter();

	void BeginBuyingUpgrades();
	void BuySingleUpgrade(const int upgradeID, const int itemslot, const int quantity);
	void RefundUpgrades();
	void EndBuyingUpgrades();
	void FetchUpgrades();
	void FilterUpgrades();
	void RemoveFinishedUpgrades();
	void RemoveSingleUpgradeInfo(const TF2BotUpgradeInfo_t* info);
	void AdvancePriority() { ++m_nextpriority; }
	void ExecuteBuy();
	void ExecuteRefund();
	bool CanAffordAnyUpgrade() const;
	bool AnyUpgradeAvailable() const;
};

#endif // !NAVBOT_TF2_MVM_UPGRADES_H_
