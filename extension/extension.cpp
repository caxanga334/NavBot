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

#include NAVBOT_PCH_FILE
#include <filesystem>
#include <string>

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
#include "sourcepawn/misc.h"

#if defined(EXT_DEBUG)
#include <tier1/KeyValues.h>
#endif // defined(EXT_DEBUG)

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

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
ISpatialPartition* partition = nullptr;

IBinTools* g_pBinTools = nullptr;
ISDKTools* g_pSDKTools = nullptr;
ISDKHooks* g_pSDKHooks = nullptr;

CBaseEntityList* g_EntList = nullptr;

IBotManager* botmanager = nullptr;
NavBotExt* extension = nullptr;
CExtManager* extmanager = nullptr;
NavBotExt g_NavBotExt;		/**< Global singleton for extension's main interface */


ConVar smnav_version("sm_navbot_version", SMEXT_CONF_VERSION, FCVAR_NOTIFY | FCVAR_DONTRECORD | FCVAR_SPONLY, "Extension version convar.");

#if defined(EXT_DEBUG) && SOURCE_ENGINE >= SE_EYE
static ConVar cvar_dump_kv_cmds("sm_navbot_debug_dump_kv_commands", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "If enabled, prints keyvalue commands sent by clients into the console.");
#endif // defined(EXT_DEBUG) && SOURCE_ENGINE >= SE_EYE


static_assert(sizeof(Vector) == 12, "Size of Vector class is not 12 bytes (3 x 4 bytes float)!");

SH_DECL_HOOK1_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool);
SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t*, const CCommand&);

#if defined(EXT_DEBUG) && SOURCE_ENGINE >= SE_EYE
SH_DECL_HOOK2_void(IServerGameClients, ClientCommandKeyValues, SH_NOATTRIB, 0, edict_t*, KeyValues*);
#endif // defined(EXT_DEBUG) && SOURCE_ENGINE >= SE_EYE

// SDKs that requires a runplayercommand hook

SMEXT_LINK(&g_NavBotExt);

namespace Utils
{
	inline static void CreateDataDirectory(const char* mod)
	{
		std::unique_ptr<char[]> path = std::make_unique<char[]>(PLATFORM_MAX_PATH);
		smutils->BuildPath(SourceMod::Path_SM, path.get(), PLATFORM_MAX_PATH, "data/navbot/%s", mod);

		auto exists = std::filesystem::exists(path.get());

		if (!exists)
		{
			auto result = std::filesystem::create_directories(path.get());

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
		std::unique_ptr<char[]> path = std::make_unique<char[]>(PLATFORM_MAX_PATH);
		smutils->BuildPath(SourceMod::Path_SM, path.get(), PLATFORM_MAX_PATH, "configs/navbot/%s", mod);

		auto exists = std::filesystem::exists(path.get());

		if (!exists)
		{
			auto result = std::filesystem::create_directories(path.get());

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

NavBotExt::NavBotExt()
{
	m_hookruncmd = false;
	m_botsAreSupported = true; // default to true, set to false on failures
	m_cfg_navbot = nullptr;
	m_cfg_sdktools = nullptr;
	m_cfg_sdkhooks = nullptr;
}

bool NavBotExt::SDK_OnLoad(char* error, size_t maxlen, bool late)
{
	extension = this;
	m_hookruncmd = false;
	m_cfg_navbot = nullptr;
	m_cfg_sdkhooks = nullptr;
	m_cfg_sdktools = nullptr;
	// generate a random seed for the global generators
	librandom::ReSeedGlobalGenerators();

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
		smutils->LogError(myself, "Failed to load SDKHooks gamedata!");
		gameconfs->CloseGameConfigFile(m_cfg_sdkhooks);
		return false;
	}

	const char* value = m_cfg_navbot->GetKeyValue("HookPlayerRunCMD");

	if (value)
	{
		int i = atoi(value);
		m_hookruncmd = (i != 0);
	}

	value = m_cfg_navbot->GetKeyValue("NoBots");

	if (value)
	{
		int i = atoi(value);

		if (i != 0)
		{
			smutils->LogMessage(myself, "Bot support disabled: NoBots == true on gamedata file.");
			m_botsAreSupported = false;
		}
	}

	int offset = 0;

	if (m_hookruncmd)
	{
		smutils->LogMessage(myself, "CBasePlayer::PlayerRunCommand hook enabled!");
	}

	if (!CBaseBot::InitHooks(m_cfg_navbot, m_cfg_sdkhooks, m_cfg_sdktools))
	{
		smutils->LogMessage(myself, "Bot support disabled: Failed to setup virtual hooks used by bots.");
		m_botsAreSupported = false;
	}

	if (gamehelpers->GetGlobalEntityList() == nullptr)
	{
		ke::SafeSprintf(error, maxlen, "NULL g_EntList from IGameHelpers!");
		return false;
	}

	// Init will grab the offset values from the gamedata files.
	if (!sdkcalls->Init())
	{
		smutils->LogMessage(myself, "Bot support disabled: Failed to setup virtual SDK Calls used by bots.");
		m_botsAreSupported = false;
	}

	value = m_cfg_navbot->GetKeyValue("FailIfNoBots");

	if (value && atoi(value) != 0 && !m_botsAreSupported)
	{
		ke::SafeSprintf(error, maxlen, "Bot support is not available but gamedata tell us that bots should be supported for this mod!");
		return false;
	}

	// This stuff needs to be after any load failures so we don't causes other stuff to crash
	ConVar_Register(0, this);
	playerhelpers->AddClientListener(this);
	sharesys->AddDependency(myself, "bintools.ext", true, true);
	sharesys->AddDependency(myself, "sdktools.ext", true, true);
	sharesys->AddDependency(myself, "sdkhooks.ext", true, true);
	sharesys->RegisterLibrary(myself, "navbot");
	spmisc::RegisterNavBotMultiTargetFilter();

	return true;
}

void NavBotExt::SDK_OnUnload()
{
	spmisc::UnregisterNavBotMultiTargetFilter();
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

		if (!m_botsAreSupported)
		{
			extmanager->NotifyBotsAreUnsupported();
		}
	}

	if (TheNavMesh == nullptr)
	{
		auto mod = extmanager->GetMod();
		TheNavMesh = mod->NavMeshFactory();

		try
		{
			TheNavMesh->LoadEditSounds(m_cfg_navbot);
		}
		catch (const std::exception& ex)
		{
			smutils->LogError(myself, "Exception throw by CNavMesh::LoadEditSounds! %s", ex.what());
		}
	}

	entprops->Init(true);
	sdkcalls->PostInit(); // setup the calls

#ifndef NO_SOURCEPAWN_API
	natives::setup(m_natives);
	natives::bots::setup(m_natives);
	natives::bots::interfaces::path::setup(m_natives);
	m_natives.push_back({ nullptr, nullptr });
	sharesys->AddNatives(myself, m_natives.data());
	smutils->LogMessage(myself, "Registered %i natives.", m_natives.size() - 1);
#endif // !NO_SOURCEPAWN_API
}

bool NavBotExt::SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetServerFactory, servergameents, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);
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
	GET_V_IFACE_CURRENT(GetEngineFactory, partition, ISpatialPartition, INTERFACEVERSION_SPATIALPARTITION);

	debugoverlay = reinterpret_cast<IVDebugOverlay*>(ismm->VInterfaceMatch(ismm->GetEngineFactory(), VDEBUG_OVERLAY_INTERFACE_VERSION));

	// Warn if debug overlay is not available on a Listen Server.
	if (debugoverlay == nullptr && !engine->IsDedicatedServer())
	{
		ismm->LogMsg(this, "Warning: Could not get interface %s. Nav mesh drawing and editing will not be available.", VDEBUG_OVERLAY_INTERFACE_VERSION);
	}
#ifdef EXT_DEBUG
	else
	{
		ismm->LogMsg(this, "Found debug overlay interface.");
	}
#endif // EXT_DEBUG

	GET_V_IFACE_CURRENT(GetServerFactory, botmanager, IBotManager, INTERFACEVERSION_PLAYERBOTMANAGER);

	gpGlobals = ismm->GetCGlobals();

	g_pCVar = icvar; // TO-DO: add #if source engine here

	SH_ADD_HOOK(IServerGameDLL, GameFrame, servergamedll, SH_MEMBER(this, &NavBotExt::Hook_GameFrame), true);
	SH_ADD_HOOK(IServerGameClients, ClientCommand, gameclients, SH_MEMBER(this, &NavBotExt::Hook_ClientCommand), true);

#ifdef EXT_DEBUG

#if SOURCE_ENGINE >= SE_EYE
	SH_ADD_HOOK(IServerGameClients, ClientCommandKeyValues, gameclients, SH_MEMBER(this, &NavBotExt::Hook_ClientCommandKeyValues), true);
#endif // SOURCE_ENGINE >= SE_EYE

#endif // EXT_DEBUG

	return true;
}

bool NavBotExt::SDK_OnMetamodUnload(char* error, size_t maxlen)
{
	SH_REMOVE_HOOK(IServerGameDLL, GameFrame, servergamedll, SH_MEMBER(this, &NavBotExt::Hook_GameFrame), false);
	SH_REMOVE_HOOK(IServerGameClients, ClientCommand, gameclients, SH_MEMBER(this, &NavBotExt::Hook_ClientCommand), true);

#ifdef EXT_DEBUG

#if SOURCE_ENGINE >= SE_EYE
	SH_REMOVE_HOOK(IServerGameClients, ClientCommandKeyValues, gameclients, SH_MEMBER(this, &NavBotExt::Hook_ClientCommandKeyValues), true);
#endif // SOURCE_ENGINE >= SE_EYE

#endif // EXT_DEBUG

	return true;
}

void NavBotExt::OnCoreMapStart(edict_t* pEdictList, int edictCount, int clientMax)
{
	librandom::ReSeedGlobalGenerators();
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
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("NavBotExt::Hook_GameFrame", "NavBot");
#endif // EXT_VPROF_ENABLED

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

void NavBotExt::Hook_ClientCommand(edict_t* pEntity, const CCommand& args)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("NavBotExt::Hook_ClientCommand", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (extmanager != nullptr && UtilHelpers::IsValidEdict(pEntity))
	{
		auto player = playerhelpers->GetGamePlayer(pEntity);

		if (player != nullptr && player->IsInGame())
		{
			extmanager->OnClientCommand(pEntity, player, args);
		}
	}

	RETURN_META(MRES_IGNORED);
}

#if defined(EXT_DEBUG) && SOURCE_ENGINE >= SE_EYE
void NavBotExt::Hook_ClientCommandKeyValues(edict_t* pEntity, KeyValues* pKeyValues)
{
	if (cvar_dump_kv_cmds.GetBool())
	{
		KeyValuesDumpAsDevMsg(pKeyValues);
	}

	RETURN_META(MRES_IGNORED);
}
#endif // EXT_DEBUG

