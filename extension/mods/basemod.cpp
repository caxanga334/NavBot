#include <extension.h>
#include <extplayer.h>
#include <bot/basebot.h>
#include "basemod.h"

CBaseMod::CBaseMod()
{
}

CBaseMod::~CBaseMod()
{
}

CBaseExtPlayer* CBaseMod::AllocatePlayer(edict_t* edict)
{
#ifndef NDEBUG
	auto index = gamehelpers->IndexOfEdict(edict);
	smutils->LogMessage(myself, "CBaseMod::AllocatePlayer new player allocated! %i %p", index, this);
#endif // !NDEBUG	

	return new CBaseExtPlayer(edict);
}

CBaseBot* CBaseMod::AllocateBot(edict_t* edict)
{
#ifndef SMNAV_FEAT_BOT
	return nullptr;
#else
	return new CBaseBot(edict);
#endif // !SMNAV_FEAT_BOT
}
