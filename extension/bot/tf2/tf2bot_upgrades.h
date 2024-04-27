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
		STATE_GOBUY = 0, // Start buying stuff
		STATE_BUYINGSTUFF, // Buying stuff
		STATE_ADVANCE, // Advance into the next priority
		STATE_WAITFORNEXTWAVE, // Already bought stuff, wait until the bot gets more money from the next wave
		STATE_DOREFUND, // Bot should refund and rebuy stuff
		STATE_REFILTER, // Rerun the weapon filter

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
		m_state = STATE_GOBUY;
	}

	void OnUpgradesRefunded()
	{
		m_tobuylist.clear();
		m_numupgrades = 0;
		m_state = STATE_GOBUY;
		InitUpgrades();
	}

	void OnDoneUpgrading()
	{
		m_state = STATE_WAITFORNEXTWAVE;
	}

	// Called when a wave ends
	void OnWaveEnd()
	{
		m_state = STATE_GOBUY;
	}

	// Called when a wave is failed
	void OnWaveFailed()
	{
		// It's hard to keep track of upgrades during wave failures, just refund everything and start again
		m_state = STATE_DOREFUND;
	}

	void InitUpgrades()
	{
		m_nextpriority = 1;
		FetchUpgrades();
	}

	TF2BotUpgrade_t* GetOrCreateUpgradeData(const MvMUpgrade_t* upgrade);
	void DoRefund() { m_state = STATE_DOREFUND; }

	const int GetCurrentPriority() const { return m_nextpriority; }
	const bool IsUpgradingDone() const { return m_tobuylist.empty(); }
	const bool IsAtInitialPriority() const { return m_nextpriority == INITIAL_PRIORITY; }
	const bool ShouldGoToAnUpgradeStation() const { return m_state == STATE_GOBUY || m_state == STATE_DOREFUND || m_state == STATE_REFILTER || m_state == STATE_ADVANCE || 
		m_state == STATE_BUYINGSTUFF; }
	const bool IsDoneForCurrentWave() const { return m_state == STATE_WAITFORNEXTWAVE; }
	const bool IsBuyingUpgrades() const { return m_state == STATE_GOBUY || m_state == STATE_BUYINGSTUFF; }
	BuyUpgradeState GetState() const { return m_state; }

private:
	CTF2Bot* m_me;
	int m_numupgrades;
	int m_nextpriority;
	std::vector<const TF2BotUpgradeInfo_t*> m_tobuylist;
	std::vector<TF2BotUpgrade_t> m_boughtlist;
	BuyUpgradeState m_state;

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
