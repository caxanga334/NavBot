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
	virtual ~CTeamFortress2Mod();

	static CTeamFortress2Mod* GetTF2Mod();

	virtual const char* GetModName() override { return "Team Fortress 2"; }

	virtual Mods::ModType GetModType() override { return Mods::ModType::MOD_TF2; }
	virtual void OnMapStart() override;
	virtual void OnMapEnd() override;
	virtual void RegisterGameEvents() override;

	virtual CBaseBot* AllocateBot(edict_t* edict) override;
	virtual CNavMesh* NavMeshFactory() override;

	inline const CWeaponInfoManager& GetWeaponInfoManager() const { return m_wim; }
	inline TeamFortress2::GameModeType GetCurrentGameMode() const { return m_gamemode; }
	const char* GetCurrentGameModeName() const;
	TeamFortress2::TFWeaponID GetWeaponID(std::string& classname);
	const WeaponInfo* GetWeaponInfo(edict_t* weapon);
	bool ShouldSwitchClass(CTF2Bot* bot) const;
	TeamFortress2::TFClassType SelectAClassForBot(CTF2Bot* bot) const;
	TeamFortress2::TeamRoles GetTeamRole(TeamFortress2::TFTeam team) const;
	CTF2ClassSelection::ClassRosterType GetRosterForTeam(TeamFortress2::TFTeam team) const;
	edict_t* GetFlagToFetch(TeamFortress2::TFTeam team);
private:
	CWeaponInfoManager m_wim;
	TeamFortress2::GameModeType m_gamemode; // Current detected game mode for the map
	std::unordered_map<std::string, TeamFortress2::TFWeaponID> m_weaponidmap;
	CTF2ClassSelection m_classselector;

	void DetectCurrentGameMode();
	bool DetectMapViaName();
	bool DetectMapViaGameRules();
	bool DetectKoth();
	bool DetectPlayerDestruction();
};

inline TeamFortress2::TFWeaponID CTeamFortress2Mod::GetWeaponID(std::string& classname)
{
	auto it = m_weaponidmap.find(classname);

	if (it != m_weaponidmap.end())
	{
		return it->second;
	}

	return TeamFortress2::TFWeaponID::TF_WEAPON_NONE;
}

#endif // !SMNAV_TF2_MOD_H_
