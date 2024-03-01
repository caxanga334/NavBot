#ifndef SMNAV_TF2_MOD_H_
#define SMNAV_TF2_MOD_H_
#pragma once

#include <unordered_map>
#include <string>
#include <bot/interfaces/weaponinfo.h>
#include <mods/basemod.h>
#include "tf2_class_selection.h"
#include "teamfortress2_shareddefs.h"

class CTF2Bot;
struct edict_t;

class CTeamFortress2Mod : public CBaseMod
{
public:
	CTeamFortress2Mod();
	~CTeamFortress2Mod() override;

	static CTeamFortress2Mod* GetTF2Mod();

	const char* GetModName() override { return "Team Fortress 2"; }

	Mods::ModType GetModType() override { return Mods::ModType::MOD_TF2; }
	void OnMapStart() override;
	void OnMapEnd() override;
	void RegisterGameEvents() override;

	CBaseBot* AllocateBot(edict_t* edict) override;
	CNavMesh* NavMeshFactory() override;

	int GetWeaponEconIndex(edict_t* weapon) const override;
	int GetWeaponID(edict_t* weapon) const override;

	inline TeamFortress2::GameModeType GetCurrentGameMode() const { return m_gamemode; }
	const char* GetCurrentGameModeName() const;
	TeamFortress2::TFWeaponID GetTFWeaponID(std::string& classname) const;
	bool ShouldSwitchClass(CTF2Bot* bot) const;
	TeamFortress2::TFClassType SelectAClassForBot(CTF2Bot* bot) const;
	TeamFortress2::TeamRoles GetTeamRole(TeamFortress2::TFTeam team) const;
	CTF2ClassSelection::ClassRosterType GetRosterForTeam(TeamFortress2::TFTeam team) const;
	edict_t* GetFlagToFetch(TeamFortress2::TFTeam team);
private:
	TeamFortress2::GameModeType m_gamemode; // Current detected game mode for the map
	std::unordered_map<std::string, TeamFortress2::TFWeaponID> m_weaponidmap;
	CTF2ClassSelection m_classselector;

	void DetectCurrentGameMode();
	bool DetectMapViaName();
	bool DetectMapViaGameRules();
	bool DetectKoth();
	bool DetectPlayerDestruction();
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
