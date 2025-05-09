#ifndef EXT_BASE_MOD_H_
#define EXT_BASE_MOD_H_

#include <memory>
#include <optional>
#include <string>
#include <sdkports/eventlistenerhelper.h>
#include <sdkports/sdk_ehandle.h>
#include <bot/interfaces/weaponinfo.h>
#include <bot/interfaces/profile.h>
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

public:
	// Gets the cleaned up current map name used for loading config files.
	virtual std::string GetCurrentMapName() const;
	// Called once after the manager has allocated the mod class
	virtual void PostCreation();
	// Called every server frame
	virtual void Frame() {}
	// Called at intervals
	virtual void Update() {}
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
	virtual void OnRoundStart() {}
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

	virtual void OnClientCommand(edict_t* pEdict, SourceMod::IGamePlayer* player, const CCommand& args) {}
protected:
	// SMC parser data
	int m_parser_depth;
	std::unique_ptr<CModSettings> m_modsettings;
	std::unique_ptr<CWeaponInfoManager> m_weaponinfomanager;
	std::unique_ptr<CDifficultyManager> m_profilemanager;

private:
	CBaseHandle m_playerresourceentity;

	void InternalFindPlayerResourceEntity();
};

#endif // !EXT_BASE_MOD_H_
