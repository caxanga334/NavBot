#ifndef NAVBOT_MVM_UPGRADE_PARSER_H_
#define NAVBOT_MVM_UPGRADE_PARSER_H_
#pragma once

#include <vector>
#include <unordered_map>
#include <ITextParsers.h>

#include "mvm_upgrade.h"
#include "teamfortress2_shareddefs.h"

class KeyValues;

class CMvMBotUpgradeInfoStorage
{
public:
	CMvMBotUpgradeInfoStorage() { m_upgradeinfos.reserve(32); m_highestpriority = 0; }

	std::vector<TF2BotUpgradeInfo_t> m_upgradeinfos;
	int m_highestpriority;

	const int GetMaxPriority() const { return m_highestpriority; }
};

class CMvMUpgradeManager : public SourceMod::ITextListener_SMC
{
public:
	CMvMUpgradeManager() :
		m_parserinfo()
	{
		m_upgrades.reserve(64);
		m_lastindex = 0;
		m_parserinupgradesection = false;
		m_parserclass = TeamFortress2::TFClass_Unknown;
	}

	~CMvMUpgradeManager() {}

	static constexpr int MVM_DEFAULT_QUALITY = 2;

	virtual SourceMod::SMCResult ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name) override;
	virtual SourceMod::SMCResult ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value) override;
	virtual SourceMod::SMCResult ReadSMC_LeavingSection(const SourceMod::SMCStates* states) override;

	void Reset()
	{
		m_lastindex = 0;
		m_upgrades.clear();
		m_upgradeinfos.clear();
		m_parserclass = TeamFortress2::TFClass_Unknown;
		m_parserinupgradesection = false;
	}

	void ParseUpgradeFile();
	void ParseBotUpgradeInfoFile();
	void ConCommand_ListUpgrades() const;

	void Reload();

	/**
	 * @brief Searches for an MvM Upgrade
	 * @param attributename Attribute to search for
	 * @param quality Upgrade quality to search for
	 * @return Poiner to upgrade definition or NULL if none found
	 */
	const MvMUpgrade_t* FindUpgrade(const std::string& attributename, int quality = MVM_DEFAULT_QUALITY) const;
	void CollectUpgradesToBuy(std::vector<const TF2BotUpgradeInfo_t*>& upgradeVec, int priority, TeamFortress2::TFClassType classtype) const;

	/**
	 * @brief Returns the max priority for a given class
	 * @param classtype TF Class
	 * @return Max priority or -1 on failure
	 */
	const int GetMaxPriorityForClass(TeamFortress2::TFClassType classtype) const
	{
		auto storage = m_upgradeinfos.find(static_cast<int>(classtype));

		if (storage != m_upgradeinfos.end())
		{
			return storage->second.GetMaxPriority();
		}

		return -1;
	}

private:
	std::vector<MvMUpgrade_t> m_upgrades;
	std::unordered_map<int, CMvMBotUpgradeInfoStorage> m_upgradeinfos;
	int m_lastindex;
	TF2BotUpgradeInfo_t m_parserinfo;
	TeamFortress2::TFClassType m_parserclass;
	bool m_parserinupgradesection;

	void ParseUpgradeBlock(KeyValues* kv);

	void InitUpgradeInfo()
	{
		m_upgradeinfos.emplace(static_cast<int>(TeamFortress2::TFClass_Scout), CMvMBotUpgradeInfoStorage{});
		m_upgradeinfos.emplace(static_cast<int>(TeamFortress2::TFClass_Soldier), CMvMBotUpgradeInfoStorage{});
		m_upgradeinfos.emplace(static_cast<int>(TeamFortress2::TFClass_Pyro), CMvMBotUpgradeInfoStorage{});
		m_upgradeinfos.emplace(static_cast<int>(TeamFortress2::TFClass_DemoMan), CMvMBotUpgradeInfoStorage{});
		m_upgradeinfos.emplace(static_cast<int>(TeamFortress2::TFClass_Heavy), CMvMBotUpgradeInfoStorage{});
		m_upgradeinfos.emplace(static_cast<int>(TeamFortress2::TFClass_Engineer), CMvMBotUpgradeInfoStorage{});
		m_upgradeinfos.emplace(static_cast<int>(TeamFortress2::TFClass_Medic), CMvMBotUpgradeInfoStorage{});
		m_upgradeinfos.emplace(static_cast<int>(TeamFortress2::TFClass_Sniper), CMvMBotUpgradeInfoStorage{});
		m_upgradeinfos.emplace(static_cast<int>(TeamFortress2::TFClass_Spy), CMvMBotUpgradeInfoStorage{});
	}

	void PostLoadBotUpgradeInfo();

	void ClearParserTemporaryInfo()
	{
		m_parserinfo.allowedweapons.clear();
		m_parserinfo.excludedweapons.clear();
		m_parserinfo.attribute.clear();
		m_parserinfo.itemslot = 0;
		m_parserinfo.maxlevel = -1;
		m_parserinfo.priority = 1;
		m_parserinfo.quality = MVM_DEFAULT_QUALITY;
	}
};

#endif // !NAVBOT_MVM_UPGRADE_PARSER_H_
