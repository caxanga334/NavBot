#ifndef EXT_BASE_MOD_H_
#define EXT_BASE_MOD_H_

#include <memory>
#include <optional>
#include <ITextParsers.h>
#include <sdkports/eventlistenerhelper.h>

class CBaseExtPlayer;
class CBaseBot;
class CNavMesh;

#include "gamemods_shared.h"
#include "modsettings.h"

// Base game mod class
class CBaseMod : public CEventListenerHelper, public ITextListener_SMC
{
public:
	CBaseMod();
	virtual ~CBaseMod();

	static constexpr auto NO_ECON_INDEX = -1;
	static constexpr auto NO_WEAPON_ID = -1;

	// Called when a game event is fired
	void FireGameEvent(IGameEvent* event) override {}

	void ReadSMC_ParseStart() override 
	{
		m_parser_depth = 0;
	}

	void ReadSMC_ParseEnd(bool halted, bool failed) override {}

	SourceMod::SMCResult ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name) override;

	SourceMod::SMCResult ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value) override;

	SourceMod::SMCResult ReadSMC_LeavingSection(const SourceMod::SMCStates* states) override;

	SourceMod::SMCResult ReadSMC_RawLine(const SourceMod::SMCStates* states, const char* line) override { return SourceMod::SMCResult_Continue; }

	// Creates the mod settings object, override to use mod specific mod settings
	virtual CModSettings* CreateModSettings() const { return new CModSettings; }

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
	// Returns the weapon ID, used for quick identification of the weapon
	virtual int GetWeaponID(edict_t* weapon) const { return NO_WEAPON_ID; }
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
protected:
	// SMC parser data
	int m_parser_depth;

private:
	CBaseHandle m_playerresourceentity;
	std::unique_ptr<CModSettings> m_modsettings;
	void InternalFindPlayerResourceEntity();

	void ParseModSettings();
};

#endif // !EXT_BASE_MOD_H_
