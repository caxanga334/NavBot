#ifndef EXT_BASE_MOD_H_
#define EXT_BASE_MOD_H_

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <sdkports/eventlistenerhelper.h>
#include <sdkports/sdk_ehandle.h>
#include <bot/interfaces/weaponinfo.h>
#include <bot/interfaces/profile.h>
#include <bot/interfaces/sharedmemory.h>
#include <IPlayerHelpers.h>
#include "gamemods_shared.h"
#include "modsettings.h"

class CNavMesh;
class CBaseBot;

// Base game mod class
class CBaseMod : public CEventListenerHelper
{
public:
	CBaseMod();
	virtual ~CBaseMod();

	/**
	 * @brief Utility class for propagating game events to bots.
	 * 
	 * Moddata is optional and can be NULL.
	 */
	class PropagateGameEventToBots
	{
	public:
		PropagateGameEventToBots()
		{
			moddata = nullptr;
			event = nullptr;
		}

		void operator()(CBaseBot* bot);

		void* moddata;
		const IGameEvent* event;
	};

	static constexpr auto NO_ECON_INDEX = -1;

protected:

	// Called when a game event is fired
	void FireGameEvent(IGameEvent* event) override {}

	// Creates the mod settings object, override to use mod specific mod settings
	virtual CModSettings* CreateModSettings() const { return new CModSettings; }
	// Creates the mod weapon info manager object, override to use a custom class
	virtual CWeaponInfoManager* CreateWeaponInfoManager() const { return new CWeaponInfoManager; }
	// Creates the bot difficulty profile manager object, override to use a custom class
	virtual CDifficultyManager* CreateBotDifficultyProfileManager() const { return new CDifficultyManager; }
	// Creates a new shared memory object
	virtual ISharedBotMemory* CreateSharedMemory() const { return new ISharedBotMemory; }
public:
	// Gets the cleaned up current map name used for loading config files.
	virtual std::string GetCurrentMapName() const;
	// Called once after the manager has allocated the mod class
	virtual void PostCreation();
	// Called every server frame
	virtual void Frame();
	// Called at intervals
	virtual void Update();
	// Called when the map starts
	virtual void OnMapStart();
	// Called when the map ends
	virtual void OnMapEnd() {}
	// Called by the manager when allocating a new bot instance
	virtual CBaseBot* AllocateBot(edict_t* edict);
	// Mod name (IE: Team Fortress 2)
	virtual const char* GetModName() { return "CBaseMod"; }
	// Mod ID
	virtual Mods::ModType GetModType() { return Mods::ModType::MOD_BASE; }
	// Allocates the nav mesh class used by the mod
	virtual CNavMesh* NavMeshFactory();
	// Returns the entity index of the player resource/manager entity.
	virtual std::optional<int> GetPlayerResourceEntity();
	// Returns the economy item index for the given weapon if the mod uses it (IE: TF2)
	virtual int GetWeaponEconIndex(edict_t* weapon) const { return NO_ECON_INDEX; }
	// True if the given client should not count towards the number of clients in the server when checking the bot quota
	virtual bool BotQuotaIsClientIgnored(int client, edict_t* entity, SourceMod::IGamePlayer* player) const { return false; }
	// A new round has started
	virtual void OnNewRound() {}
	// A round has started
	virtual void OnRoundStart();
	// A round has ended
	virtual void OnRoundEnd() {}
	// Called when the nav mesh is fully loaded and valid.
	virtual void OnNavMeshLoaded() {}
	// Called when the nav mesh is destroyed. Any stored nav mesh data is invalid at this point. Use this to clear anything that's stored on the mod.
	virtual void OnNavMeshDestroyed() {}

	// Mod settings data. Unavailable until a map is loaded.
	const CModSettings* GetModSettings() const { return m_modsettings.get(); }
	// Reparses the mod settings file.
	void ReloadModSettingsFile();
	// Reloads the weapon info config file
	void ReloadWeaponInfoConfigFile();
	// Mod weapon info manager
	const CWeaponInfoManager* GetWeaponInfoManager() const { return m_weaponinfomanager.get(); }
	void ReloadBotDifficultyProfile();
	// Bot profile difficulty manager
	const CDifficultyManager* GetBotDifficultyManager() const { return m_profilemanager.get(); }
	/**
	 * @brief Called by the client command hook.
	 * @param pEdict Edict pointer of the player who sent the command.
	 * @param player Sourcemod player pointer of the player who sent the command.
	 * @param args The arguments passed to the command.
	 */
	virtual void OnClientCommand(edict_t* pEdict, SourceMod::IGamePlayer* player, const CCommand& args) {}
	/**
	 * @brief Gets a pointer to the bot shared memory interface for the given team index.
	 * @param teamindex Team index number.
	 * @return Shared bot memory interface pointer.
	 */
	virtual ISharedBotMemory* GetSharedBotMemory(int teamindex);
	/**
	 * @brief Checks if there are no solid obstructions for hitscan weapons between the given positions.
	 * @param from Raycast trace start position.
	 * @param to Raycast trace end position.
	 * @param passEnt optional entity to pass to the trace filter.
	 * @return True if no obstruction is found, false otherwise.
	 */
	virtual bool IsLineOfFireClear(const Vector& from, const Vector& to, CBaseEntity* passEnt = nullptr) const;
protected:
	std::unique_ptr<CModSettings> m_modsettings;
	std::unique_ptr<CWeaponInfoManager> m_weaponinfomanager;
	std::unique_ptr<CDifficultyManager> m_profilemanager;

private:
	CBaseHandle m_playerresourceentity;
	std::array<std::unique_ptr<ISharedBotMemory>, MAX_TEAMS> m_teamsharedmemory;

	void InternalFindPlayerResourceEntity();
};

#endif // !EXT_BASE_MOD_H_
