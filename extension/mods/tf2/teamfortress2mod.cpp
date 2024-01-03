#include <extension.h>
#include <manager.h>
#include <extplayer.h>
#include <bot/basebot.h>
#include <bot/tf2/tf2bot.h>
#include <core/eventmanager.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include "teamfortress2mod.h"

CTeamFortress2Mod::CTeamFortress2Mod() : CBaseMod()
{
    m_wim.LoadConfigFile();
}

CTeamFortress2Mod::~CTeamFortress2Mod()
{
}

CTeamFortress2Mod* CTeamFortress2Mod::GetTF2Mod()
{
    return static_cast<CTeamFortress2Mod*>(extmanager->GetMod());
}

CBaseBot* CTeamFortress2Mod::AllocateBot(edict_t* edict)
{
    return new CTF2Bot(edict);
}

CNavMesh* CTeamFortress2Mod::NavMeshFactory()
{
    return new CTFNavMesh;
}
