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

	virtual const char* GetModName() { return "Team Fortress 2"; }

	virtual Mods::ModType GetModType() { return Mods::ModType::MOD_TF2; }

	virtual CBaseBot* AllocateBot(edict_t* edict);
	virtual CNavMesh* NavMeshFactory();

	inline const CWeaponInfoManager& GetWeaponInfoManager() { return m_wim; }

private:
	CWeaponInfoManager m_wim;
};

#endif // !SMNAV_TF2_MOD_H_
