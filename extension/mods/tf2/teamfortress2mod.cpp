#include <extension.h>
#include <extplayer.h>
#include <bot/basebot.h>
#include <bot/tf2/tf2bot.h>
#include <core/eventmanager.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include "teamfortress2mod.h"

CTeamFortress2Mod::CTeamFortress2Mod() : CBaseMod()
{
}

CTeamFortress2Mod::~CTeamFortress2Mod()
{
}

CBaseBot* CTeamFortress2Mod::AllocateBot(edict_t* edict)
{
    return new CTF2Bot(edict);
}

CNavMesh* CTeamFortress2Mod::NavMeshFactory()
{
    return new CTFNavMesh;
}
