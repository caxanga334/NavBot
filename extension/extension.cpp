/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include <filesystem>

#include "extension.h"
#include <engine/ivdebugoverlay.h>
#include <engine/IEngineTrace.h>
#include <iplayerinfo.h>
#include <vphysics_interface.h>
#include <filesystem.h>
#include <datacache/imdlcache.h>
#include <igameevents.h>
#include <toolframework/itoolentity.h>

#include <ISDKTools.h>
#include <IBinTools.h>
#include <ISDKHooks.h>

#include "navmesh/nav_mesh.h"
#include "manager.h"
#include <util/entprops.h>

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

CGlobalVars* gpGlobals = nullptr;
IVDebugOverlay* debugoverlay = nullptr;
// IVEngineServer* engine = nullptr;
IServerGameDLL* servergamedll = nullptr;
IServerGameEnts* servergameents = nullptr;
IEngineTrace* enginetrace = nullptr;
CNavMesh* TheNavMesh = nullptr;
IPlayerInfoManager* playerinfomanager = nullptr;
IServerGameClients* gameclients = nullptr;
IGameEventManager* gameeventmanager1 = nullptr;
IGameEventManager2* gameeventmanager = nullptr;
IPhysicsSurfaceProps* physprops = nullptr;
IVModelInfo* modelinfo = nullptr;
IMDLCache* mdlcache = nullptr;
IFileSystem* filesystem = nullptr;
ICvar* icvar = nullptr;
IServerTools* servertools = nullptr;

IBinTools* g_pBinTools = nullptr;
ISDKTools* g_pSDKTools = nullptr;
ISDKHooks* g_pSDKHooks = nullptr;

#ifdef SMNAV_FEAT_BOT
IBotManager* botmanager = nullptr;
#endif // SMNAV_FEAT_BOT

CExtManager* extmanager = nullptr;

SMNavExt g_SMNavExt;		/**< Global singleton for extension's main interface */

static_assert(sizeof(Vector) == 12, "Size of Vector class is not 12 bytes (3 x 4 bytes float)!");

SH_DECL_HOOK1_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool);

SMEXT_LINK(&g_SMNavExt);

namespace Utils
{
	inline void CreateDataDirectory(const char* mod)
	{
		char fullpath[PLATFORM_MAX_PATH + 1];
		smutils->BuildPath(SourceMod::Path_SM, fullpath, sizeof(fullpath), "data/smnav/%s", mod);

		auto exists = std::filesystem::exists(fullpath);

		if (!exists)
		{
			auto result = std::filesystem::create_directories(fullpath);

			if (!result)
			{
				smutils->LogError(myself, "Failed to create data directory!");
			}
			else
			{
				smutils->LogMessage(myself, "Data directory created!");
			}
		}
	}

	inline void CreateConfigDirectory(const char* mod)
	{
		char fullpath[PLATFORM_MAX_PATH + 1];
		smutils->BuildPath(SourceMod::Path_SM, fullpath, sizeof(fullpath), "configs/smnav/%s", mod);

		auto exists = std::filesystem::exists(fullpath);

		if (!exists)
		{
			auto result = std::filesystem::create_directories(fullpath);

			if (!result)
			{
				smutils->LogError(myself, "Failed to create configs directory!");
			}
			else
			{
				smutils->LogMessage(myself, "Configs directory created!");
			}
		}
	}
}


bool SMNavExt::SDK_OnLoad(char* error, size_t maxlen, bool late)
{
	// Create the directory
	auto mod = smutils->GetGameFolderName();

	Utils::CreateDataDirectory(mod);
	Utils::CreateConfigDirectory(mod);

	ConVar_Register(0, this);

	playerhelpers->AddClientListener(this);

	return true;
}

void SMNavExt::SDK_OnUnload()
{
	delete TheNavMesh;
	TheNavMesh = nullptr;

	delete extmanager;
	extmanager = nullptr;

	ConVar_Unregister();
}

void SMNavExt::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);
	SM_GET_LATE_IFACE(SDKTOOLS, g_pSDKTools);
	SM_GET_LATE_IFACE(SDKHOOKS, g_pSDKHooks);

	if (TheNavMesh == nullptr)
	{
		TheNavMesh = NavMeshFactory();
	}

	if (extmanager == nullptr)
	{
		extmanager = new CExtManager;
		extmanager->OnAllLoaded();
	}

	entprops->Init(true);
}

bool SMNavExt::SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	GET_V_IFACE_ANY(GetEngineFactory, enginetrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER);
	// GET_V_IFACE_ANY(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_ANY(GetEngineFactory, gameeventmanager1, IGameEventManager, INTERFACEVERSION_GAMEEVENTSMANAGER);
	GET_V_IFACE_ANY(GetEngineFactory, gameeventmanager, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	GET_V_IFACE_ANY(GetEngineFactory, physprops, IPhysicsSurfaceProps, VPHYSICS_SURFACEPROPS_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, modelinfo, IVModelInfo, VMODELINFO_SERVER_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, mdlcache, IMDLCache, MDLCACHE_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, servergamedll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_ANY(GetServerFactory, playerinfomanager, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);
	GET_V_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_ANY(GetServerFactory, servertools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION);

#ifndef __linux__
	GET_V_IFACE_ANY(GetEngineFactory, debugoverlay, IVDebugOverlay, VDEBUG_OVERLAY_INTERFACE_VERSION);
#endif

#ifdef SMNAV_FEAT_BOT
	GET_V_IFACE_ANY(GetServerFactory, botmanager, IBotManager, INTERFACEVERSION_PLAYERBOTMANAGER);
#endif // SMNAV_FEAT_BOT

	gpGlobals = ismm->GetCGlobals();

	g_pCVar = icvar; // TO-DO: add #if source engine here

	SH_ADD_HOOK(IServerGameDLL, GameFrame, servergamedll, SH_MEMBER(this, &SMNavExt::Hook_GameFrame), false);

	return true;
}

bool SMNavExt::SDK_OnMetamodUnload(char* error, size_t maxlen)
{
	SH_REMOVE_HOOK(IServerGameDLL, GameFrame, servergamedll, SH_MEMBER(this, &SMNavExt::Hook_GameFrame), false);

	return true;
}

void SMNavExt::OnCoreMapStart(edict_t* pEdictList, int edictCount, int clientMax)
{
	auto error = TheNavMesh->Load();

	if (error != NAV_OK)
	{
		rootconsole->ConsolePrint("Nav Mesh failed to load!");
	}
}

void SMNavExt::OnCoreMapEnd()
{
}

bool SMNavExt::RegisterConCommandBase(ConCommandBase* pVar)
{
	return META_REGCVAR(pVar);
}

void SMNavExt::OnClientPutInServer(int client)
{
	extmanager->OnClientPutInServer(client);
}

void SMNavExt::OnClientDisconnecting(int client)
{
	extmanager->OnClientDisconnect(client);
}

void SMNavExt::Hook_GameFrame(bool simulating)
{
	if (TheNavMesh)
	{
		TheNavMesh->Update();
	}

	if (extmanager)
	{
		extmanager->Frame();
	}

	RETURN_META(MRES_IGNORED);
}