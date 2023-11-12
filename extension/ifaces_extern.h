#ifndef INTERFACES_EXTERN_H_
#define INTERFACES_EXTERN_H_
#pragma once

#include <extension.h>
#include <engine/ivdebugoverlay.h>
#include <engine/IEngineTrace.h>
#include <iplayerinfo.h>
#include <vphysics_interface.h>
#include <filesystem.h>
#include <datacache/imdlcache.h>
#include <igameevents.h>
#include <toolframework/itoolentity.h>
#include "navmesh/nav_mesh.h"
#include "manager.h"
#include <ISDKTools.h>
#include <IBinTools.h>
#include <ISDKHooks.h>

extern CGlobalVars* gpGlobals;
extern IVDebugOverlay* debugoverlay;
// extern IVEngineServer* engine;
extern IServerGameDLL* servergamedll;
extern IServerGameEnts* servergameents;
extern IEngineTrace* enginetrace;
extern CNavMesh* TheNavMesh;
extern IPlayerInfoManager* playerinfomanager;
extern IServerGameClients* gameclients;
extern IGameEventManager* gameeventmanager1;
extern IGameEventManager2* gameeventmanager;
extern IPhysicsSurfaceProps* physprops;
extern IVModelInfo* modelinfo;
extern IMDLCache* mdlcache;
extern IFileSystem* filesystem;
extern ICvar* icvar;
extern IServerTools* servertools;

extern IBinTools* g_pBinTools;
extern ISDKTools* g_pSDKTools;
extern ISDKHooks* g_pSDKHooks;

#ifdef SMNAV_FEAT_BOT
extern IBotManager* botmanager;
#endif // SMNAV_FEAT_BOT

extern CExtManager* extmanager;

#endif // !INTERFACES_EXTERN_H_

