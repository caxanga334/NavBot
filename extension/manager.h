#ifndef SMNAV_EXT_MANAGER_H_
#define SMNAV_EXT_MANAGER_H_
#pragma once

#include <cstdint>
#include <bot/interfaces/profile.h>
#include <sdkports/sdk_timers.h>
#include <IForwardSys.h>
#include "pawn_mem_manager.h"

class CBaseMod;
class CBaseExtPlayer;
class CBaseBot;

#if SOURCE_ENGINE == SE_EPISODEONE
#define IConVar ConVar
#endif // SOURCE_ENGINE == SE_EPISODEONE


// Primary Extension Manager
class CExtManager
{
public:
	enum class BotQuotaMode : uint8
	{
		QUOTA_FIXED = 0, // Manager keeps a fixed number of bots
		QUOTA_FILL, // Fill with N bots, automatically kicked to free space for humans
	};

	enum class BotCreateMethod : std::uint8_t
	{
		CREATEFAKECLIENT = 0U,
		CREATEFAKECLIENTEX,
		BOTMANAGER,

		MAX_METHODS
	};

	static constexpr float MOD_UPDATE_INTERVAL = 1.0f; // interval between CBaseMod::Update calls
	static constexpr float MOD_UPDATE_AFTER_MAP_LOAD = 5.0f; // interval to call CBaseMod::Update after a level change

	CExtManager();
	~CExtManager();

	// Called when all extensions have been loaded
	void OnAllLoaded();
	// Called every server frame
	void Frame();

	void OnClientPutInServer(int client);

	void OnClientDisconnect(int client);

	void OnMapStart();
	void OnMapEnd();

	void AllocateMod();

	CBaseMod* GetMod();

	CBaseBot* GetBotByIndex(int index);
	CBaseBot* GetBotFromEntity(CBaseEntity* entity);
	/**
	 * @brief Find bots by name.
	 * @param name Full or partial name.
	 * @return Bot pointer if found or NULL.
	 */
	CBaseBot* FindBotByName(const char* name) const;
	bool IsNavBot(const int client) const;

	void AddBot(std::string* newbotname = nullptr, edict_t** newbotedict = nullptr);
	CBaseBot* AttachBotInstanceToEntity(edict_t* entity);
	void RemoveRandomBot(const char* message);
	void RemoveAllBots(const char* message) const;

	// Gets a vector of all bots currently in game
	const auto &GetAllBots() const { return m_bots; }

	void LoadBotNames();

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
	 * @tparam T A class with operator() overload with one parameter (CBaseBot* bot).
	 * @param functor Function to call on each bot.
	 */
	template <typename T>
	inline void ForEachBot(T& functor)
	{
		for (auto& ptr : m_bots)
		{
			CBaseBot* bot = ptr.get();
			functor(bot);
		}
	}

	void SetBotQuotaMode(BotQuotaMode mode) { m_quotamode = mode; }
	void SetBotQuotaTarget(int target) { m_quotatarget = target; }

	static void OnQuotaModeCvarChanged(IConVar* var, const char* pOldValue, float flOldValue);
	static void OnQuotaTargetCvarChanged(IConVar* var, const char* pOldValue, float flOldValue);

	CSourcePawnMemoryManager* GetPawnMemoryManager() const { return m_pawnmemory.get(); }
	
	// Gets the value of sv_gravity cached at map start
	static const float GetSvGravityValue() { return CExtManager::s_sv_gravity; }

	void OnClientCommand(edict_t* pEdict, SourceMod::IGamePlayer* player, const CCommand& args);
	// Tells the manager that bots cannot be added due to lack of support
	void NotifyBotsAreUnsupported() { m_allowbots = false; }
	bool AreBotsSupported() const { return m_allowbots; }

	static BotCreateMethod GetBotCreateMethod() { return s_botcreatemethod; }
	static bool ShouldFixUpBotFlags() { return s_fixupbotflags; }
	// Utility function that provides auto completion of in-game NavBot names
	static int AutoComplete_BotNames(const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

private:
	std::vector<std::unique_ptr<CBaseBot>> m_bots; // Vector of bots
	std::vector<std::string> m_botnames; // Vector of names to be used by bots
	std::unique_ptr<CBaseMod> m_mod; // mod pointer
	std::unique_ptr<CSourcePawnMemoryManager> m_pawnmemory;
#ifndef NO_SOURCEPAWN_API
	SourceMod::IForward* m_prebotaddforward; // SM Forward, on pre bot add
	SourceMod::IForward* m_postbotaddforward; // SM Forward, post bot add (normal bots)
	SourceMod::IForward* m_prepluginbotaddforward; // SM Forward, on pre plugin bot add
	SourceMod::IForward* m_postpluginbotaddforward; // SM Forward, post bot add (plugin bots)  
#endif // !NO_SOURCEPAWN_API

	size_t m_nextbotname; // Index of the next bot name to use
	int m_botdebugmode;
	int m_quotaupdatetime; // Bot quota timer
	BotQuotaMode m_quotamode; // Bot quota mode
	int m_quotatarget; // Bot quota target
	bool m_iscreatingbot; // We are creating a NavBot
	bool m_allowbots; // allow bots to be created
	CountdownTimer m_callModUpdateTimer; // timer for calling the mod update function

	// Getting horrible performance at vstdlib.dll from a function called by ConVarRef::Init, so we are caching the sv_gravity value here
	static inline float s_sv_gravity{ 800.0f };
	static inline BotCreateMethod s_botcreatemethod{ BotCreateMethod::BOTMANAGER };
	static inline bool s_fixupbotflags{ false };
};

extern CExtManager* extmanager;

#endif // !SMNAV_EXT_MANAGER_H_
