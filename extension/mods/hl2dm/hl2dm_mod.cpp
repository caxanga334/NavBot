#include NAVBOT_PCH_FILE
#include <extension.h>
#include <bot/hl2dm/hl2dmbot.h>
#include "hl2dm_mod.h"

CHalfLife2DeathMatchMod::CHalfLife2DeathMatchMod() :
	CBaseMod()
{
}

CHalfLife2DeathMatchMod::~CHalfLife2DeathMatchMod()
{
}

CBaseBot* CHalfLife2DeathMatchMod::AllocateBot(edict_t* edict)
{
	return new CHL2DMBot(edict);
}
