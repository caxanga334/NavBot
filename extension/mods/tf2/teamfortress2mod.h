#ifndef SMNAV_TF2_MOD_H_
#define SMNAV_TF2_MOD_H_
#pragma once

#include "teamfortress2_shareddefs.h"

#include <mods/basemod.h>


class CTeamFortress2Mod : public CBaseMod
{
public:
	CTeamFortress2Mod();
	virtual ~CTeamFortress2Mod();

	virtual const char* GetModName() { return "Team Fortress 2"; }

	virtual Mods::ModType GetModType() { return Mods::ModType::MOD_TF2; }

	virtual CBaseBot* AllocateBot(edict_t* edict);
	virtual CNavMesh* NavMeshFactory();
};

#endif // !SMNAV_TF2_MOD_H_
