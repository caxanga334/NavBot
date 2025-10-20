#include NAVBOT_PCH_FILE
#include <bot/zps/zpsbot.h>
#include "zps_mod.h"

CZombiePanicSourceMod::CZombiePanicSourceMod()
{
}

CBaseBot* CZombiePanicSourceMod::AllocateBot(edict_t* edict)
{
	return new CZPSBot(edict);
}
