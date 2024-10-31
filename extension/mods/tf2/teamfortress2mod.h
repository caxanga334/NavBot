#ifndef SMNAV_TF2_MOD_H_
#define SMNAV_TF2_MOD_H_
#pragma once

#include <array>
#include <unordered_map>
#include <string>
#include <bot/interfaces/weaponinfo.h>
#include <mods/basemod.h>
#include <sdkports/sdk_ehandle.h>
#include "tf2_class_selection.h"
#include "teamfortress2_shareddefs.h"
#include "mvm_upgrade_manager.h"

class CTF2Bot;
struct edict_t;

class CTeamFortress2Mod : public CBaseMod
{
public:
	CTeamFortress2Mod();
	~CTeamFortress2Mod() override;

	static CTeamFortress2Mod* GetTF2Mod();

	void FireGameEvent(IGameEvent* event) override;

	const char* GetModName() override { return "Team Fortress 2"; }

	Mods::ModType GetModType() override { return Mods::ModType::MOD_TF2; }
	void OnMapStart() override;
	void OnMapEnd() override;

	CBaseBot* AllocateBot(edict_t* edict) override;
	CNavMesh* NavMeshFactory() override;

	int GetWeaponEconIndex(edict_t* weapon) const override;
	int GetWeaponID(edict_t* weapon) const override;
	bool BotQuotaIsClientIgnored(int client, edict_t* entity, SourceMod::IGamePlayer* player) const override;

	inline TeamFortress2::GameModeType GetCurrentGameMode() const { return m_gamemode; }
	const char* GetCurrentGameModeName() const;
	TeamFortress2::TFWeaponID GetTFWeaponID(std::string& classname) const;
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
	void CollectControlPointsToAttack(CTF2Bot* bot, std::vector<CBaseEntity*>& out);
	void CollectControlPointsToDefend(CTF2Bot* bot, std::vector<CBaseEntity*>& out);
	CBaseEntity* GetControlPointByIndex(const int index) const;

	void DebugInfo_ControlPoints();

#ifdef EXT_DEBUG
	inline void Debug_UpdatePayload()
	{
		FindPayloadCarts();
	}
#endif // EXT_DEBUG

	void ShowControlPoints() const;

private:
	TeamFortress2::GameModeType m_gamemode; // Current detected game mode for the map
	std::unordered_map<std::string, TeamFortress2::TFWeaponID> m_weaponidmap;
	CTF2ClassSelection m_classselector;
	CMvMUpgradeManager m_upgrademanager;
	CHandle<CBaseEntity> m_red_payload;
	CHandle<CBaseEntity> m_blu_payload;
	std::array<CHandle<CBaseEntity>, TeamFortress2::TF_MAX_CONTROL_POINTS> m_controlpoints;
	CHandle<CBaseEntity> m_objecteResourceEntity;
	TeamFortress2::TFObjectiveResource m_objectiveResourcesData;
	bool m_bInSetup;

	void DetectCurrentGameMode();
	bool DetectMapViaName();
	bool DetectMapViaGameRules();
	bool DetectKoth();
	bool DetectPlayerDestruction();

	void FindPayloadCarts();
	void FindControlPoints();
	void CheckForSetup();
	void UpdateObjectiveResource();
	bool TeamMayCapturePoint(int team, int pointindex) const;
};

inline TeamFortress2::TFWeaponID CTeamFortress2Mod::GetTFWeaponID(std::string& classname) const
{
	auto it = m_weaponidmap.find(classname);

	if (it != m_weaponidmap.end())
	{
		return it->second;
	}

	return TeamFortress2::TFWeaponID::TF_WEAPON_NONE;
}

#endif // !SMNAV_TF2_MOD_H_
