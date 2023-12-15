#include <extension.h>
#include <mods/tf2/teamfortress2mod.h>
#include <ifaces_extern.h>
#include "tf2bot.h"

CTFBot::CTFBot(edict_t* edict) : CBaseBot(edict)
{
}

CTFBot::~CTFBot()
{
}

void CTFBot::TryJoinGame()
{
	FakeClientCommand("jointeam auto");
	FakeClientCommand("joinclass soldier");
}
