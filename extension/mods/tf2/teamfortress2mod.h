#ifndef SMNAV_TF2_MOD_H_
#define SMNAV_TF2_MOD_H_
#pragma once

#include <bot/interfaces/weaponinfo.h>
#include <mods/basemod.h>

#include "teamfortress2_shareddefs.h"

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

	virtual CBaseBot* AllocateBot(edict_t* edict) override;
	virtual CNavMesh* NavMeshFactory() override;

	inline const CWeaponInfoManager& GetWeaponInfoManager() const { return m_wim; }
	inline TeamFortress2::GameModeType GetCurrentGameMode() const { return m_gamemode; }
	const char* GetCurrentGameModeName() const;
private:
	CWeaponInfoManager m_wim;
	TeamFortress2::GameModeType m_gamemode; // Current detected game mode for the map

	void DetectCurrentGameMode();
	bool DetectMapViaName();
	bool DetectMapViaGameRules();
	bool DetectKoth();
	bool DetectPlayerDestruction();
};

#endif // !SMNAV_TF2_MOD_H_
