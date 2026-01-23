#ifndef SMNAV_EXT_MANAGER_H_
#define SMNAV_EXT_MANAGER_H_
#pragma once

#include <cstdint>
#include <bot/bot_debug_shared.h>
#include <bot/interfaces/profile.h>
#include <bot/basebot.h>
#include <sdkports/sdk_timers.h>
#include <IForwardSys.h>
#include <IGameConfigs.h>
#include "servercommand_mgr.h"

class CBaseMod;
class CBaseExtPlayer;
class CBaseBot;

// Primary Extension Manager
class CExtManager
{
public:
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
	/**
	 * @brief Called when a client in put in server.
	 * @param client Client index.
	 */
	void OnClientPutInServer(int client);
	/**
	 * @brief Called when a client disconnects from the server.
	 * @param client Client index.
	 */
	void OnClientDisconnect(int client);
	// Called when a new map loads
	void OnMapStart();
	// Called when the current map unloads.
	void OnMapEnd();
	// Called to allocate the mod interface, this is also where the mod detection code is.
	void AllocateMod();
	// Returns a pointer to the mod interface.
	CBaseMod* GetMod();
	/**
	 * @brief Gets a bot pointer by client/entity index.
	 * @param index Client/Entity index to search.
	 * @return Bot of the given index, NULL if none.
	 */
	CBaseBot* GetBotByIndex(int index) const;
	/**
	 * @brief Gets a player pointer by client/entity index.
	 * @param index Client/Entity index to search.
	 * @return Player pointer or NULL if not found.
	 */
	CBaseExtPlayer* GetPlayerByIndex(int index) const;
	/**
	 * @brief Gets the bot pointer of the given entity.
	 * @param entity Entity to search.
	 * @return Bot pointer if the given entity is of a NavBot instance. NULL if the given entity is not a NavBot instance.
	 */
	CBaseBot* GetBotFromEntity(CBaseEntity* entity) const;
	/**
	 * @brief Gets the player pointer of the given entity.
	 * @param entity Entity to search.
	 * @return player pointer if the given entity is of a NavBot instance. NULL if the given entity is not a player with an allocated Player instance.
	 */
	CBaseExtPlayer* GetPlayerOfEntity(CBaseEntity* entity) const;
	/**
	 * @brief Gets the player pointer of the given edict_t pointer.
	 * @param edict Edict_t to search.
	 * @return player pointer if the given entity is of a NavBot instance. NULL if the given edict is not a player with an allocated Player instance.
	 */
	CBaseExtPlayer* GetPlayerOfEdict(edict_t* edict) const;
	// Returns a player pointer of the listen server host. Always returns NULL on dedicated servers.
	CBaseExtPlayer* GetListenServerHost() const;
	/**
	 * @brief Find bots by name.
	 * @param name Full or partial name.
	 * @return Bot pointer if found or NULL.
	 */
	CBaseBot* FindBotByName(const char* name) const;
	// Returns true if the given client index is a NavBot instance.
	bool IsNavBot(const int client) const;
	/**
	 * @brief Adds a new NavBot instance.
	 * @param newbotname Optional bot name override.
	 * @param newbotedict Optional pointer to get the edict of the bot added.
	 */
	void AddBot(std::string* newbotname = nullptr, edict_t** newbotedict = nullptr);
	CBaseBot* AttachBotInstanceToEntity(edict_t* entity);
	/**
	 * @brief Removes a random bot from the game.
	 * @param message Kick reason message.
	 */
	void RemoveRandomBot(const char* message);
	/**
	 * @brief Removes all NavBot bots from the game.
	 * @param message Kick reason message.
	 */
	void RemoveAllBots(const char* message) const;
	// Gets a vector of all bots currently in game
	const auto &GetAllBots() const { return m_bots; }
	void LoadBotNames();
	void StopAllDebugging() { m_botdebugmode = 0; }
	void ToggleDebugOption(int bits) { m_botdebugmode = m_botdebugmode ^ bits; }
	bool IsDebugging(int bits) const { return (m_botdebugmode & bits) ? true : false; }
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
	/**
	 * @brief Runs a function on each player instance.
	 * @tparam T A class with operator() overload with one parameter void (CBaseExtPlayer* player).
	 * @param functor Function to run on each player.
	 */
	template <typename T>
	void ForEachPlayer(T& functor)
	{
		for (auto& ptr : m_players)
		{
			CBaseExtPlayer* player = ptr.get();
			functor(player);
		}
	}
	/**
	 * @brief Runs a function on each player and bot instance.
	 * @tparam T A class with operator() overload with one parameter void (CBaseExtPlayer* player).
	 * @param functor Function to run on each player/bot.
	 */
	template <typename T>
	void ForEachClient(T& functor)
	{
		for (auto& ptr : m_bots)
		{
			CBaseBot* bot = ptr.get();
			functor(static_cast<CBaseExtPlayer*>(bot));
		}

		for (auto& ptr : m_players)
		{
			CBaseExtPlayer* player = ptr.get();
			functor(player);
		}
	}
	
	// Gets the value of sv_gravity cached at map start
	static const float GetSvGravityValue() { return CExtManager::s_sv_gravity; }

#if SOURCE_ENGINE <= SE_DARKMESSIAH
	// Source 2006
	void OnClientCommand_OldEngine(edict_t* pEdict, SourceMod::IGamePlayer* player);
#else
	// Source 2007 and newer
	void OnClientCommand(edict_t* pEdict, SourceMod::IGamePlayer* player, const CCommand& args);
#endif // SOURCE_ENGINE <= SE_DARKMESSIAH

	// Tells the manager that bots cannot be added due to lack of support
	void NotifyBotsAreUnsupported() { m_allowbots = false; }
	bool AreBotsSupported() const { return m_allowbots; }

	static BotCreateMethod GetBotCreateMethod() { return s_botcreatemethod; }
	static bool ShouldFixUpBotFlags() { return s_fixupbotflags; }
	// Utility function that provides auto completion of in-game NavBot names
	static int AutoComplete_BotNames(const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

	CServerCommandManager& GetServerCommandManager() { return m_serverCommandManager; }

	// If true, the current game/mod has support for loading maps from steam workshop.
	static bool ModUsesWorkshopMaps() { return s_usesworkshop; }
	static void SetModUsesWorkshopMaps(bool state) { s_usesworkshop = state; }
	// If true, prefer using unique map names for workshop maps
	static bool ShouldPreferUniqueMapNames() { return s_preferuniquemapnames; }
	static void SetShouldPreferUniqueMapNames(bool state) { s_preferuniquemapnames = state; }
	static bool ParseGamedata(SourceMod::IGameConfig* gamedata);

private:
	std::vector<std::unique_ptr<CBaseBot>> m_bots; // Vector of bots
	std::vector<std::unique_ptr<CBaseExtPlayer>> m_players; // Vector of non NavBot players
	std::vector<std::string> m_botnames; // Vector of names to be used by bots
	std::unique_ptr<CBaseMod> m_mod; // mod pointer
#ifndef NO_SOURCEPAWN_API
	SourceMod::IForward* m_prebotaddforward; // SM Forward, on pre bot add
	SourceMod::IForward* m_postbotaddforward; // SM Forward, post bot add (normal bots)
	SourceMod::IForward* m_prepluginbotaddforward; // SM Forward, on pre plugin bot add
	SourceMod::IForward* m_postpluginbotaddforward; // SM Forward, post bot add (plugin bots)
#endif // !NO_SOURCEPAWN_API

	size_t m_nextbotname; // Index of the next bot name to use
	int m_botdebugmode;
	bool m_iscreatingbot; // We are creating a NavBot
	bool m_allowbots; // allow bots to be created
	CountdownTimer m_callModUpdateTimer; // timer for calling the mod update function
	CServerCommandManager m_serverCommandManager;

	// Getting horrible performance at vstdlib.dll from a function called by ConVarRef::Init, so we are caching the sv_gravity value here
	static inline float s_sv_gravity{ 800.0f };
	static inline BotCreateMethod s_botcreatemethod{ BotCreateMethod::BOTMANAGER };
	static inline bool s_fixupbotflags{ false };
	static inline bool s_usesworkshop{ false };
	static inline bool s_preferuniquemapnames{ false };
};

extern CExtManager* extmanager;

#endif // !SMNAV_EXT_MANAGER_H_
