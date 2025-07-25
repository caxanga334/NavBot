#ifndef SMNAV_TF2_MOD_H_
#define SMNAV_TF2_MOD_H_
#pragma once

#include <array>
#include <unordered_map>
#include <string>
#include <sdkports/sdk_timers.h>
#include <bot/tf2/tf2bot_weaponinfo.h>
#include <mods/basemod.h>
#include <sdkports/sdk_ehandle.h>
#include "tf2_class_selection.h"
#include "teamfortress2_shareddefs.h"
#include "mvm_upgrade_manager.h"

class CTF2ModSettings : public CModSettings
{
public:
	CTF2ModSettings() : CModSettings()
	{
		engineer_nest_dispenser_range = 900.0f;
		engineer_nest_exit_range = 1200.0f;
		entrance_spawn_range = 2048.0f;
		mvm_sentry_to_bomb_range = 1500.0f;
		medic_patient_scan_range = 1024.0f;
		engineer_destroy_travel_range = 4500.0f;
		engineer_move_check_interval = 60.0f;
		engineer_sentry_killassist_threshold = 5;
		engineer_teleporter_uses_threshold = 4;
		engineer_help_ally_max_range = 1500.0f;
	}

	void SetEngineerNestDispenserRange(float range) { engineer_nest_dispenser_range = range; }
	void SetEngineerNestExitRange(float range) { engineer_nest_exit_range = range; }
	void SetEntranceSpawnRange(float range) { entrance_spawn_range = range; }
	void SetMvMSentryToBombRange(float range) { mvm_sentry_to_bomb_range = range; }
	void SetMedicPatientScanRange(float range) { medic_patient_scan_range = range; }
	void SetEngineerMoveDestroyBuildingRange(float range) { engineer_destroy_travel_range = range; }
	void SetEngineerMoveCheckInterval(float value) { engineer_move_check_interval = value; }
	void SetEngineerSentryKillAssistsThreshold(int value) { engineer_sentry_killassist_threshold = value; }
	void SetEngineerTeleporterUsesThreshold(int value) { engineer_teleporter_uses_threshold = value; }
	void SetEngineerHelpAllyMaxRange(float value) { engineer_help_ally_max_range = value; }

	float GetEngineerNestDispenserRange() const { return engineer_nest_dispenser_range; }
	float GetEngineerNestExitRange() const { return engineer_nest_exit_range; }
	float GetEntranceSpawnRange() const { return entrance_spawn_range; }
	float GetMvMSentryToBombRange() const { return mvm_sentry_to_bomb_range; }
	float GetMedicPatientScanRange() const { return medic_patient_scan_range; }
	float GetEngineerMoveDestroyBuildingRange() const { return engineer_destroy_travel_range; }
	float GetEngineerMoveCheckInterval() const { return engineer_move_check_interval; }
	int GetEngineerSentryKillAssistsThreshold() const { return engineer_sentry_killassist_threshold; }
	int GetEngineerTeleporterUsesThreshold() const { return engineer_teleporter_uses_threshold; }
	float GetEngineerHelpAllyMaxRange() const { return engineer_help_ally_max_range; }

protected:
	SourceMod::SMCResult ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value) override;

private:
	float engineer_nest_dispenser_range; // maximum distance between the dispenser and the sentry gun
	float engineer_nest_exit_range; // maximum distance between the teleporter exit and the sentry gun
	float entrance_spawn_range; // maximum distance between the teleporter entrance and the active spawn point
	float mvm_sentry_to_bomb_range; // MvM: maximum sentry build distance
	float medic_patient_scan_range; // distance medics will scan for patients
	float engineer_destroy_travel_range; // When moving buildings, if the travel distance is larger than this, destroy it instead
	float engineer_move_check_interval;
	int engineer_sentry_killassist_threshold;
	int engineer_teleporter_uses_threshold;
	float engineer_help_ally_max_range;
};

class CTF2Bot;
class CTFWaypoint;
struct edict_t;

class CTeamFortress2Mod : public CBaseMod
{
public:
	CTeamFortress2Mod();
	~CTeamFortress2Mod() override;

	static CTeamFortress2Mod* GetTF2Mod();

protected:

	void FireGameEvent(IGameEvent* event) override;
	virtual CModSettings* CreateModSettings() const override { return new CTF2ModSettings; }
	CWeaponInfoManager* CreateWeaponInfoManager() const override { return new CTF2WeaponInfoManager; }

public:
	virtual std::string GetCurrentMapName() const;

#if SOURCE_ENGINE == SE_TF2
	static void OnForceGameModeConVarChanged(IConVar* var, const char* pOldValue, float flOldValue);
#endif // SOURCE_ENGINE == SE_TF2

	void Update() override;

	const char* GetModName() override { return "Team Fortress 2"; }

	Mods::ModType GetModType() override { return Mods::ModType::MOD_TF2; }
	void OnMapStart() override;
	void OnMapEnd() override;

	CBaseBot* AllocateBot(edict_t* edict) override;
	CNavMesh* NavMeshFactory() override;

	int GetWeaponEconIndex(edict_t* weapon) const override;
	bool BotQuotaIsClientIgnored(int client, edict_t* entity, SourceMod::IGamePlayer* player) const override;

	void OnNavMeshLoaded() override;
	void OnNavMeshDestroyed() override;

	inline TeamFortress2::GameModeType GetCurrentGameMode() const { return m_gamemode; }
	const char* GetCurrentGameModeName() const;
	// This function checks if class changes are allowed (IE: MvM after wave start)
	bool IsAllowedToChangeClasses() const;
	bool ShouldSwitchClass(CTF2Bot* bot) const;
	TeamFortress2::TFClassType SelectAClassForBot(CTF2Bot* bot) const;
	TeamFortress2::TeamRoles GetTeamRole(TeamFortress2::TFTeam team) const;
	CTF2ClassSelection::ClassRosterType GetRosterForTeam(TeamFortress2::TFTeam team) const;
	edict_t* GetFlagToFetch(TeamFortress2::TFTeam team);
	const CMvMUpgradeManager& GetMvMUpgradeManager() const { return m_upgrademanager; }
	void ReloadUpgradeManager() { m_upgrademanager.Reload(); }
	void OnRoundStart() override;
	CBaseEntity* GetREDPayload() const { return m_red_payload.Get(); }
	CBaseEntity* GetBLUPayload() const { return m_blu_payload.Get(); }
	bool IsInSetup() const { return m_bInSetup; }
	const TeamFortress2::TFObjectiveResource* GetTFObjectiveResource() const;
	void CollectControlPointsToAttack(TeamFortress2::TFTeam tfteam, std::vector<CBaseEntity*>& out);
	void CollectControlPointsToDefend(TeamFortress2::TFTeam tfteam, std::vector<CBaseEntity*>& out);
	CBaseEntity* GetControlPointByIndex(const int index) const;
	/**
	 * @brief Gets the active round timer.
	 * @param team For which team? (Only used in KOTH game mode).
	 * @return Timer entity or NULL if no timer is found.
	 */
	CBaseEntity* GetRoundTimer(TeamFortress2::TFTeam team = TeamFortress2::TFTeam::TFTeam_Unassigned) const;

	void DebugInfo_ControlPoints();

#ifdef EXT_DEBUG
	inline void Debug_UpdatePayload()
	{
		FindPayloadCarts();
	}
#endif // EXT_DEBUG

	void Command_ShowControlPoints() const;

	const std::vector<CTFWaypoint*>& GetAllSniperWaypoints() const { return m_sniperWaypoints; }
	const std::vector<CTFWaypoint*>& GetAllSentryWaypoints() const { return m_sentryWaypoints; }
	const std::vector<CTFWaypoint*>& GetAllDispenserWaypoints() const { return m_dispenserWaypoints; }
	const std::vector<CTFWaypoint*>& GetAllTeleEntranceWaypoints() const { return m_teleentranceWaypoints; }
	const std::vector<CTFWaypoint*>& GetAllTeleExitWaypoints() const { return m_teleexitWaypoints; }

	void OnClientCommand(edict_t* pEdict, SourceMod::IGamePlayer* player, const CCommand& args) override;
	const CTF2ModSettings* GetTF2ModSettings() const { return static_cast<const CTF2ModSettings*>(CBaseMod::GetModSettings()); }
	const Vector& GetMvMBombHatchPosition() const { return m_MvMHatchPos; }
	// If true, bots will only use deathmatch behavior (including spies, medics, engineers and snipers).
	inline bool UseDeathmatchBehaviorOnly() const
	{
		switch (m_gamemode)
		{
		case TeamFortress2::GameModeType::GM_NONE:
			return false;
		case TeamFortress2::GameModeType::GM_GG:
			[[fallthrough]];
		case TeamFortress2::GameModeType::GM_DM:
			return true;
		default:
			return false;
		}
	}

	// Returns true if the halloween boss fight truce is active
	inline bool IsTruceActive() const { return m_isTruceActive; }

private:
	TeamFortress2::GameModeType m_gamemode; // Current detected game mode for the map
	std::unordered_map<std::string, int> m_weaponidmap;
	CTF2ClassSelection m_classselector;
	CMvMUpgradeManager m_upgrademanager;
	CountdownTimer m_updatePayloadTimer;
	CHandle<CBaseEntity> m_red_payload;
	CHandle<CBaseEntity> m_blu_payload;
	std::array<CHandle<CBaseEntity>, TeamFortress2::TF_MAX_CONTROL_POINTS> m_controlpoints;
	CHandle<CBaseEntity> m_objecteResourceEntity;
	TeamFortress2::TFObjectiveResource m_objectiveResourcesData;
	bool m_bInSetup;
	bool m_isTruceActive; // Is the halloween boss fight truce active?
	CountdownTimer m_setupExpireTimer; // safety timer in case the setup end event doesn't fire.
	std::vector<CTFWaypoint*> m_sniperWaypoints;
	std::vector<CTFWaypoint*> m_sentryWaypoints;
	std::vector<CTFWaypoint*> m_dispenserWaypoints;
	std::vector<CTFWaypoint*> m_teleentranceWaypoints;
	std::vector<CTFWaypoint*> m_teleexitWaypoints;
	Vector m_MvMHatchPos; // bomb hatch position in MvM
	CountdownTimer m_updateTruceStatus;

	void DetectCurrentGameMode();
	bool DetectMapViaName();
	bool DetectCommunityGameModes();
	bool DetectMapViaGameRules();
	bool DetectMapViaEntities();
	bool DetectKoth();
	bool DetectPlayerDestruction();
	void FindPayloadCarts();
	void FindControlPoints();
	void CheckForSetup();
	void UpdateObjectiveResource();
	bool TeamMayCapturePoint(int team, int pointindex) const;
	void FindMvMBombHatchPosition();
};

#endif // !SMNAV_TF2_MOD_H_
