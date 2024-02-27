#ifndef SMNAV_EXT_MANAGER_H_
#define SMNAV_EXT_MANAGER_H_
#pragma once

#include <bot/interfaces/profile.h>
#include <sdkports/sdk_timers.h>

class CBaseMod;
class CBaseExtPlayer;
class CBaseBot;

// Primary Extension Manager
class CExtManager
{
public:
	enum class BotQuotaMode : uint8
	{
		QUOTA_FIXED = 0, // Manager keeps a fixed number of bots
		QUOTA_FILL, // Fill with N bots, automatically kicked to free space for humans
	};

	CExtManager();
	~CExtManager();

	// Called when all extensions have been loaded
	void OnAllLoaded();

	void Frame();

	void OnClientPutInServer(int client);

	void OnClientDisconnect(int client);

	void OnMapStart();
	void OnMapEnd();

	void AllocateMod();

	CBaseMod* GetMod();

	void NotifyRegisterGameEvents();

	CBaseBot* GetBotByIndex(int index);
	CBaseExtPlayer* GetPlayerByIndex(int index);
	bool IsNavBot(const int client) const;

	void AddBot();
	void RemoveRandomBot();
	void RemoveAllBots(const char* message);

	// Gets a vector of all bots currently in game
	const auto &GetAllBots() const { return m_bots; }

	void LoadBotNames();

	inline const CDifficultyManager& GetBotDifficultyProfileManager() const { return m_bdm; }

	inline void StopAllDebugging() { m_botdebugmode = 0; }
	inline void ToggleDebugOption(int bits)
	{
		m_botdebugmode = m_botdebugmode ^ bits;
	}
	inline bool IsDebugging(int bits) const
	{
		return (m_botdebugmode & bits) ? true : false;
	}

	void UpdateBotQuota();

	/**
	 * @brief Runs a function on each bot.
	 * @tparam T A class with operator() overload with one parameter (BotClass* bot). BotClass is defined in the 'B' template parameter
	 * @tparam B Bot class to use, default CBaseBot.
	 * @param functor Function to call on each bot.
	 */
	template <typename T, typename B = CBaseBot>
	inline void ForEachBot(T functor)
	{
		for (auto& ptr : m_bots)
		{
			B* bot = static_cast<B*>(ptr.get());
			functor(bot);
		}
	}

private:
	std::vector<std::unique_ptr<CBaseBot>> m_bots; // Vector of bots
	std::vector<std::unique_ptr<CBaseExtPlayer>> m_players; // Vector of (human) players
	std::vector<std::string> m_botnames; // Vector of names to be used by bots
	std::unique_ptr<CBaseMod> m_mod; // mod pointer
	size_t m_nextbotname; // Index of the next bot name to use
	CDifficultyManager m_bdm; // Bot Difficulty Profile Manager
	int m_botdebugmode;
	int m_quotaupdatetime; // Bot quota timer
	BotQuotaMode m_quotamode; // Bot quota mode
	int m_quotatarget; // Bot quota target
};

extern CExtManager* extmanager;

#endif // !SMNAV_EXT_MANAGER_H_

