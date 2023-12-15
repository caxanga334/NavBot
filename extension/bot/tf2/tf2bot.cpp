#include <extension.h>
#include <mods/tf2/teamfortress2mod.h>
#include <ifaces_extern.h>
#include "tf2bot.h"

CTF2Bot::CTF2Bot(edict_t* edict) : CBaseBot(edict)
{
}

CTF2Bot::~CTF2Bot()
{
}

void CTF2Bot::TryJoinGame()
{
	FakeClientCommand("jointeam auto");
	FakeClientCommand("joinclass soldier");
}
