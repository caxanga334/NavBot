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

CBaseBot* CBaseMod::AllocateBot(edict_t* edict)
{
#ifndef SMNAV_FEAT_BOT
	return nullptr;
#else
	return new CBaseBot(edict);
#endif // !SMNAV_FEAT_BOT
}
