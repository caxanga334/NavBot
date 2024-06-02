#include <sstream>

#include <extension.h>
#include <manager.h>
#include <tier1/KeyValues.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include "mvm_upgrade_manager.h"

SourceMod::SMCResult CMvMUpgradeManager::ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name)
{
	if (strcmp(name, "TFMvMUpgrades") == 0)
	{
		return SourceMod::SMCResult_Continue; // good
	}

	if (m_parserclass == TeamFortress2::TFClass_Unknown)
	{
		auto classtype = tf2lib::GetClassTypeFromName(name);

		if (classtype == TeamFortress2::TFClass_Unknown)
		{
			smutils->LogError(myself, "Unkwnown class \"%s\".", name);
			return SourceMod::SMCResult_HaltFail;
		}

		m_parserclass = classtype;
		return SourceMod::SMCResult_Continue;
	}

	if (strcmp(name, "upgrade") == 0)
	{
		m_parserinupgradesection = true;
		ClearParserTemporaryInfo();
		return SourceMod::SMCResult_Continue;
	}

	return SourceMod::SMCResult_Continue;
}

SourceMod::SMCResult CMvMUpgradeManager::ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value)
{
	if (!m_parserinupgradesection)
	{
		smutils->LogError(myself, "Reading KV outside upgrade section!");
		SourceMod::SMCResult_Continue;
	}

	if (strncmp(key, "attribute", 9) == 0)
	{
		m_parserinfo.attribute.assign(value);
	}
	else if (strncmp(key, "quality", 7) == 0)
	{
		m_parserinfo.quality = atoi(value);
	}
	else if (strncmp(key, "priority", 8) == 0)
	{
		m_parserinfo.priority = atoi(value);
	}
	else if (strncmp(key, "slot", 4) == 0)
	{
		m_parserinfo.itemslot = atoi(value);
	}
	else if (strncmp(key, "maxlevel", 8) == 0)
	{
		m_parserinfo.maxlevel = atoi(value);
	}
	else if (strncmp(key, "weapons", 7) == 0)
	{
		std::string szValue(value);
		std::stringstream stream(szValue);
		std::string token;

		while (std::getline(stream, token, ','))
		{
			m_parserinfo.allowedweapons.emplace(atoi(token.c_str()));
		}
	}
	else if (strncmp(key, "excludedweapons", 15) == 0)
	{
		std::string szValue(value);
		std::stringstream stream(szValue);
		std::string token;

		while (std::getline(stream, token, ','))
		{
			m_parserinfo.excludedweapons.emplace(atoi(token.c_str()));
		}
	}

	return SourceMod::SMCResult_Continue;
}

SourceMod::SMCResult CMvMUpgradeManager::ReadSMC_LeavingSection(const SourceMod::SMCStates* states)
{
	if (m_parserinupgradesection && m_parserclass != TeamFortress2::TFClass_Unknown)
	{
		m_parserinupgradesection = false; // leaving upgrade section
		auto it = m_upgradeinfos.find(static_cast<int>(m_parserclass));

		if (it == m_upgradeinfos.end())
		{
			DevWarning("Upgrade Parser: Class %i not initialized!\n", static_cast<int>(m_parserclass));
			auto data = m_upgradeinfos.emplace(static_cast<int>(m_parserclass), CMvMBotUpgradeInfoStorage{});

			if (data.second)
			{
				data.first->second.m_upgradeinfos.push_back(m_parserinfo);
			}

			return SourceMod::SMCResult_Continue;
		}

		it->second.m_upgradeinfos.push_back(m_parserinfo);
		return SourceMod::SMCResult_Continue;
	}

	if (m_parserclass != TeamFortress2::TFClass_Unknown)
	{
		// Leaving class section
		m_parserclass = TeamFortress2::TFClass_Unknown;
		return SourceMod::SMCResult_Continue;
	}

	return SourceMod::SMCResult_Continue;
}

void CMvMUpgradeManager::ParseUpgradeFile()
{
	constexpr std::string_view path{ "scripts/items/mvm_upgrades.txt" };
	KeyValues* kv = new KeyValues("upgrades");

	if (!kv->LoadFromFile(filesystem, path.data()))
	{
		kv->deleteThis();
		smutils->LogError(myself, "Failed to load MvM Upgrade script!");
		return;
	}

	ParseUpgradeBlock(kv->FindKey("ItemUpgrades"));
	ParseUpgradeBlock(kv->FindKey("PlayerUpgrades"));

	rootconsole->ConsolePrint("Loaded %i MvM Upgrades from script.", m_upgrades.size());
	kv->deleteThis();
	kv = nullptr;
}

void CMvMUpgradeManager::ParseBotUpgradeInfoFile()
{	
	InitUpgradeInfo();

	char path[PLATFORM_MAX_PATH];
	smutils->BuildPath(SourceMod::Path_SM, path, sizeof(path), "configs/navbot/tf/mvm_upgrades.cfg");
	SourceMod::SMCStates states;
	auto error = textparsers->ParseFile_SMC(path, this, &states);

	if (error != SourceMod::SMCError_Okay)
	{
		smutils->LogError(myself, "Failed to parse configuration file \"%s\"!", path);
		return;
	}

	PostLoadBotUpgradeInfo();
}

void CMvMUpgradeManager::ConCommand_ListUpgrades() const
{
	rootconsole->ConsolePrint("MvM Upgrade: index \"attribute\" increment cap cost quality");
	for (auto& upgrade : m_upgrades)
	{
		rootconsole->ConsolePrint("MvM Upgrade: %i \"%s\" %3.2f %3.2f %i %i", upgrade.index, upgrade.attribute.c_str(), upgrade.increment, upgrade.cap, upgrade.cost, upgrade.quality);
	}
}

void CMvMUpgradeManager::Reload()
{
	// Clean up pointers while they are still valid
	extmanager->ForEachBot([](CBaseBot* bot) {
		CTF2Bot* tf2bot = static_cast<CTF2Bot*>(bot);
		tf2bot->GetUpgradeManager().Reset();
	});

	Reset();
	ParseUpgradeFile();
	ParseBotUpgradeInfoFile();

	extmanager->ForEachBot([](CBaseBot* bot) {
		CTF2Bot* tf2bot = static_cast<CTF2Bot*>(bot);
		tf2bot->GetUpgradeManager().Reset();
	});
}

const MvMUpgrade_t* CMvMUpgradeManager::FindUpgrade(const std::string& attributename, int quality) const
{
	for (auto& upgrade : m_upgrades)
	{
		if (upgrade.attribute == attributename && quality == upgrade.quality)
		{
			return &upgrade;
		}
	}

	return nullptr;
}

void CMvMUpgradeManager::CollectUpgradesToBuy(std::vector<const TF2BotUpgradeInfo_t*>& upgradeVec, int priority, TeamFortress2::TFClassType classtype) const
{
	auto it = m_upgradeinfos.find(static_cast<int>(classtype));

	if (it == m_upgradeinfos.end())
	{
		smutils->LogError(myself, "CMvMUpgradeManager::CollectUpgradesToBuy no upgrades available for class %s!", tf2lib::GetClassNameFromType(classtype));
		return;
	}

	auto& storage = it->second;
	
	for (auto& upgrade : storage.m_upgradeinfos)
	{
		if (upgrade.priority == priority)
		{
			upgradeVec.push_back(&upgrade);
		}
	}
}

void CMvMUpgradeManager::ParseUpgradeBlock(KeyValues* kv)
{
	if (!kv)
		return;

	for (KeyValues* data = kv->GetFirstSubKey(); data != nullptr; data = data->GetNextKey())
	{
		KeyValues* pAttrib = data->FindKey("attribute");
		KeyValues* pIncrement = data->FindKey("increment");
		KeyValues* pCap = data->FindKey("cap");
		KeyValues* pCost = data->FindKey("cost");

		if (!pAttrib || !pIncrement || !pCap || !pCost)
		{
#ifdef EXT_DEBUG
			Warning("CMvMUpgradeManager::ParseUpgradeBlock skipping \"%s\"! \n", kv->GetName());
#endif // EXT_DEBUG

			continue; // Invalid upgrade, skip
		}

		auto attribute = data->GetString("attribute");

		if (attribute == nullptr || !attribute[0])
		{
#ifdef EXT_DEBUG
			Warning("CMvMUpgradeManager::ParseUpgradeBlock skipping \"%s\", invalid attribute! \n", kv->GetName());
#endif // EXT_DEBUG

			continue;
		}

		MvMUpgrade_t& upgrade = m_upgrades.emplace_back();
		upgrade.index = m_lastindex;
		upgrade.attribute.assign(attribute);
		upgrade.increment = data->GetFloat("increment");
		upgrade.cap = data->GetFloat("cap");
		upgrade.cost = data->GetInt("cost");
		upgrade.quality = data->GetInt("quality", MVM_DEFAULT_QUALITY);
		upgrade.FindMaxPurchases();
		m_lastindex++;
	}
}

void CMvMUpgradeManager::PostLoadBotUpgradeInfo()
{
	for (auto& storage : m_upgradeinfos)
	{
		for (auto& info : storage.second.m_upgradeinfos)
		{
			auto upgrade = FindUpgrade(info.attribute, info.quality);

			if (upgrade == nullptr)
			{
				smutils->LogError(myself, "Failed to assign an upgrade to \"%s - %i\"", info.attribute.c_str(), info.quality);
				continue;
			}

			info.SetUpgrade(upgrade);

			if (storage.second.m_highestpriority < info.priority)
			{
				storage.second.m_highestpriority = info.priority;
			}
		}
	}

	// Remove upgrade info without an assigned upgrade

	for (auto& storage : m_upgradeinfos)
	{
		auto start = std::remove_if(storage.second.m_upgradeinfos.begin(), storage.second.m_upgradeinfos.end(), [](const TF2BotUpgradeInfo_t& object) {
			if (object.upgrade == nullptr)
			{
				return true;
			}

			return false;
		});

		storage.second.m_upgradeinfos.erase(start, storage.second.m_upgradeinfos.end());
	}
}
