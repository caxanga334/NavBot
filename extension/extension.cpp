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
#include "manager.h"
#include "natives.h"
#include <util/entprops.h>
#include <util/helpers.h>
#include <util/librandom.h>
#include <util/sdkcalls.h>
#include <mods/basemod.h>
#include <bot/basebot.h>
#include <sdkports/sdk_takedamageinfo.h>

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
IEngineSound* enginesound = nullptr;
CNavMesh* TheNavMesh = nullptr;
IPlayerInfoManager* playerinfomanager = nullptr;
IServerGameClients* gameclients = nullptr;
IGameEventManager2* gameeventmanager = nullptr;
IPhysicsSurfaceProps* physprops = nullptr;
IVModelInfo* modelinfo = nullptr;
IMDLCache* imdlcache = nullptr;
IFileSystem* filesystem = nullptr;
ICvar* icvar = nullptr;
IServerTools* servertools = nullptr;
IServerPluginHelpers* serverpluginhelpers = nullptr;
IStaticPropMgrServer* staticpropmgr = nullptr;

IBinTools* g_pBinTools = nullptr;
ISDKTools* g_pSDKTools = nullptr;
ISDKHooks* g_pSDKHooks = nullptr;

CBaseEntityList* g_EntList = nullptr;

IBotManager* botmanager = nullptr;
NavBotExt* extension = nullptr;
CExtManager* extmanager = nullptr;
NavBotExt g_NavBotExt;		/**< Global singleton for extension's main interface */


ConVar smnav_version("sm_navbot_version", SMEXT_CONF_VERSION, FCVAR_NOTIFY | FCVAR_DONTRECORD | FCVAR_SPONLY, "Extension version convar.");

static_assert(sizeof(Vector) == 12, "Size of Vector class is not 12 bytes (3 x 4 bytes float)!");

SH_DECL_HOOK1_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool);

// SDKs that requires a runplayercommand hook

SMEXT_LINK(&g_NavBotExt);

namespace Utils
{
	inline static void CreateDataDirectory(const char* mod)
	{
		char fullpath[PLATFORM_MAX_PATH + 1];
		smutils->BuildPath(SourceMod::Path_SM, fullpath, sizeof(fullpath), "data/navbot/%s", mod);

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

	inline static void CreateConfigDirectory(const char* mod)
	{
		char fullpath[PLATFORM_MAX_PATH + 1];
		smutils->BuildPath(SourceMod::Path_SM, fullpath, sizeof(fullpath), "configs/navbot/%s", mod);

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

bool NavBotExt::SDK_OnLoad(char* error, size_t maxlen, bool late)
{
	extension = this;
	m_hookruncmd = false;
	m_cfg_navbot = nullptr;
	m_cfg_sdkhooks = nullptr;
	m_cfg_sdktools = nullptr;
	randomgen->ReSeed(); // set the initial seed based on the clock

	// Create the directory
	auto mod = smutils->GetGameFolderName();

	Utils::CreateDataDirectory(mod);
	Utils::CreateConfigDirectory(mod);

	if (!gameconfs->LoadGameConfigFile("navbot.games", &m_cfg_navbot, error, maxlen))
	{
		smutils->LogError(myself, "Failed to open NavBot gamedata file!");
		gameconfs->CloseGameConfigFile(m_cfg_navbot);
		return false;
	}


	if (!gameconfs->LoadGameConfigFile("sdktools.games", &m_cfg_sdktools, error, maxlen))
	{
		smutils->LogError(myself, "Failed to open SDKTools gamedata file!");
		gameconfs->CloseGameConfigFile(m_cfg_sdktools);
		return false;
	}

	if (!gameconfs->LoadGameConfigFile("sdkhooks.games", &m_cfg_sdkhooks, error, maxlen))
	{
		const char* message = "Failed to load SDKHooks gamedata!";
		maxlen = strlen(message);
		strcpy(error, message);
		gameconfs->CloseGameConfigFile(m_cfg_sdkhooks);
		return false;
	}

	auto value = m_cfg_navbot->GetKeyValue("HookPlayerRunCMD");

	if (value)
	{
		int i = atoi(value);
		m_hookruncmd = (i != 0);
	}

	int offset = 0;

	if (m_hookruncmd)
	{
		smutils->LogMessage(myself, "CBasePlayer::PlayerRunCommand hook enabled!");
	}

	if (!CBaseBot::InitHooks(m_cfg_navbot, m_cfg_sdkhooks, m_cfg_sdktools))
	{
		const char* message = "Failed to setup SourceHooks (CBaseBot)!";
		maxlen = strlen(message);
		strcpy(error, message);
		return false;
	}

	// This stuff needs to be after any load failures so we don't causes other stuff to crash
	ConVar_Register(0, this);
	playerhelpers->AddClientListener(this);
	sharesys->AddDependency(myself, "bintools.ext", true, true);
	sharesys->AddDependency(myself, "sdktools.ext", true, true);
	sharesys->AddDependency(myself, "sdkhooks.ext", true, true);
	sharesys->RegisterLibrary(myself, "navbot");
	
	return true;
}

void NavBotExt::SDK_OnUnload()
{
	gameconfs->CloseGameConfigFile(m_cfg_navbot);
	gameconfs->CloseGameConfigFile(m_cfg_sdktools);
	gameconfs->CloseGameConfigFile(m_cfg_sdkhooks);
	m_cfg_navbot = nullptr;
	m_cfg_sdktools = nullptr;
	m_cfg_sdkhooks = nullptr;

	delete TheNavMesh;
	TheNavMesh = nullptr;

	delete extmanager;
	extmanager = nullptr;

	ConVar_Unregister();
}

void NavBotExt::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);
	SM_GET_LATE_IFACE(SDKTOOLS, g_pSDKTools);
	SM_GET_LATE_IFACE(SDKHOOKS, g_pSDKHooks);

	g_EntList = reinterpret_cast<CBaseEntityList*>(gamehelpers->GetGlobalEntityList());

	if (extmanager == nullptr)
	{
		extmanager = new CExtManager;
		extmanager->OnAllLoaded();
	}

	if (TheNavMesh == nullptr)
	{
		auto mod = extmanager->GetMod();
		TheNavMesh = mod->NavMeshFactory();
		TheNavMesh->LoadEditSounds(m_cfg_navbot);
	}

	entprops->Init(true);
	sdkcalls->Init();

	natives::setup(m_natives);
	m_natives.push_back({ nullptr, nullptr });
	sharesys->AddNatives(myself, m_natives.data());
	smutils->LogMessage(myself, "Registered %i natives.", m_natives.size() - 1);
}

bool NavBotExt::SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetEngineFactory, enginetrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, enginesound, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION);
	// GET_V_IFACE_ANY(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	// IGameEventManager2 needs to be GET_V_IFACE_CURRENT or an older version will be used and things are going to crash
	GET_V_IFACE_CURRENT(GetEngineFactory, gameeventmanager, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	GET_V_IFACE_CURRENT(GetEngineFactory, physprops, IPhysicsSurfaceProps, VPHYSICS_SURFACEPROPS_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, modelinfo, IVModelInfo, VMODELINFO_SERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, imdlcache, IMDLCache, MDLCACHE_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, serverpluginhelpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);
	GET_V_IFACE_CURRENT(GetServerFactory, servergamedll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_CURRENT(GetServerFactory, playerinfomanager, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);
	GET_V_IFACE_CURRENT(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_CURRENT(GetServerFactory, servertools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, staticpropmgr, IStaticPropMgrServer, INTERFACEVERSION_STATICPROPMGR_SERVER);

#ifdef EXT_DEBUG
	if (staticpropmgr == nullptr)
	{
		smutils->LogError(myself, "Failed to get Static Prop Manager Server interface!");
	}
#endif // EXT_DEBUG


#ifdef WIN32
	GET_V_IFACE_CURRENT(GetEngineFactory, debugoverlay, IVDebugOverlay, VDEBUG_OVERLAY_INTERFACE_VERSION);
#else
	GET_V_IFACE_CURRENT(GetEngineFactory, debugoverlay, IVDebugOverlay, VDEBUG_OVERLAY_INTERFACE_VERSION);

	if (!debugoverlay && !engine->IsDedicatedServer())
	{
		// Report if debug overlay is not available on a Linux listen server
		smutils->LogError(myself, "Failed to get %s interface. NavMesh drawing will be unavailable.", VDEBUG_OVERLAY_INTERFACE_VERSION);
	}
#endif // WIN32

	GET_V_IFACE_CURRENT(GetServerFactory, botmanager, IBotManager, INTERFACEVERSION_PLAYERBOTMANAGER);

	gpGlobals = ismm->GetCGlobals();

	g_pCVar = icvar; // TO-DO: add #if source engine here

	SH_ADD_HOOK(IServerGameDLL, GameFrame, servergamedll, SH_MEMBER(this, &NavBotExt::Hook_GameFrame), false);

	return true;
}

bool NavBotExt::SDK_OnMetamodUnload(char* error, size_t maxlen)
{
	SH_REMOVE_HOOK(IServerGameDLL, GameFrame, servergamedll, SH_MEMBER(this, &NavBotExt::Hook_GameFrame), false);

	return true;
}

void NavBotExt::OnCoreMapStart(edict_t* pEdictList, int edictCount, int clientMax)
{
	extmanager->OnMapStart();
}

void NavBotExt::OnCoreMapEnd()
{
	extmanager->OnMapEnd();
}

bool NavBotExt::RegisterConCommandBase(ConCommandBase* pVar)
{
	return META_REGCVAR(pVar);
}

void NavBotExt::OnClientPutInServer(int client)
{
	extmanager->OnClientPutInServer(client);
}

void NavBotExt::OnClientDisconnecting(int client)
{
	extmanager->OnClientDisconnect(client);
}

void NavBotExt::Hook_GameFrame(bool simulating)
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

