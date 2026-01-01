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

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

class CUserCmd;
class IMoveHelper;
class CTakeDamageInfo;

/**
 * @file extension.h
 * @brief Sample extension code header.
 */

#include <vector>

#include "smsdk_ext.h"

#include <const.h>

#include <engine/ivdebugoverlay.h>
#include <engine/IEngineTrace.h>
#include <engine/IEngineSound.h>
#include <engine/IStaticPropMgr.h>
#include <iplayerinfo.h>
#include <vphysics_interface.h>
#include <filesystem.h>
#include <datacache/imdlcache.h>
#include <igameevents.h>
#include <toolframework/itoolentity.h>
#include <entitylist_base.h>
#include <ISDKTools.h>
#include <IBinTools.h>
#include <ISDKHooks.h>
#include <ispatialpartition.h>

/**
 * @brief Sample implementation of the SDK Extension.
 * Note: Uncomment one of the pre-defined virtual functions in order to use it.
 */
class NavBotExt : public SDKExtension, public IConCommandBaseAccessor, public SourceMod::IClientListener
{
public:
	NavBotExt();

	/**
	 * @brief This is called after the initial loading sequence has been processed.
	 *
	 * @param error		Error message buffer.
	 * @param maxlen	Size of error message buffer.
	 * @param late		Whether or not the module was loaded after map load.
	 * @return			True to succeed loading, false to fail.
	 */
	virtual bool SDK_OnLoad(char *error, size_t maxlen, bool late);
	
	/**
	 * @brief This is called right before the extension is unloaded.
	 */
	virtual void SDK_OnUnload();

	/**
	 * @brief This is called once all known extensions have been loaded.
	 * Note: It is is a good idea to add natives here, if any are provided.
	 */
	virtual void SDK_OnAllLoaded();

	/**
	 * @brief Called when the pause state is changed.
	 */
	//virtual void SDK_OnPauseChange(bool paused);

	/**
	 * @brief this is called when Core wants to know if your extension is working.
	 *
	 * @param error		Error message buffer.
	 * @param maxlen	Size of error message buffer.
	 * @return			True if working, false otherwise.
	 */
	//virtual bool QueryRunning(char *error, size_t maxlen);
public:
#if defined SMEXT_CONF_METAMOD
	/**
	 * @brief Called when Metamod is requesting the extension (ISmmPlugin) interface.
	 *
	 * @param mvi			Struct that contains Metamod version, SourceHook version, Plugin version and Source Engine version.
	 * @param mli			Struct that contains the library file name and path being loaded.
	 * @return				The ISmmPlugin interface.
	 */
	// virtual METAMOD_PLUGIN* SDK_OnMetamodCreateInterface(const MetamodVersionInfo* mvi, const MetamodLoaderInfo* mli);

	/**
	 * @brief Called when Metamod is attached, before the extension version is called.
	 *
	 * @param error			Error buffer.
	 * @param maxlen		Maximum size of error buffer.
	 * @param late			Whether or not Metamod considers this a late load.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late);

	/**
	 * @brief Called when Metamod is detaching, after the extension version is called.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param error			Error buffer.
	 * @param maxlen		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodUnload(char *error, size_t maxlen);

	/**
	 * @brief Called when Metamod's pause state is changing.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param paused		Pause state being set.
	 * @param error			Error buffer.
	 * @param maxlen		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	//virtual bool SDK_OnMetamodPauseChange(bool paused, char *error, size_t maxlen);
#endif

public:
	/**
	 * @brief Called on server activation before plugins receive the OnServerLoad forward.
	 *
	 * @param pEdictList		Edicts list.
	 * @param edictCount		Number of edicts in the list.
	 * @param clientMax			Maximum number of clients allowed in the server.
	 */
	virtual void OnCoreMapStart(edict_t* pEdictList, int edictCount, int clientMax);

	/**
	 * @brief Called on level shutdown
	 *
	 */
	virtual void OnCoreMapEnd();

	virtual bool RegisterConCommandBase(ConCommandBase* pVar);

	// IClientListener callbacks
public:
	/**
	 * @brief Called when a client requests connection.
	 *
	 * @param client		Index of the client.
	 * @param error			Error buffer for a disconnect reason.
	 * @param maxlength		Maximum length of error buffer.
	 * @return				True to allow client, false to reject.
	 */
	virtual bool InterceptClientConnect(int client, char* error, size_t maxlength)
	{
		return true;
	}

	/**
	 * @brief Called when a client has connected.
	 *
	 * @param client		Index of the client.
	 */
	virtual void OnClientConnected(int client)
	{
	}

	/**
	 * @brief Called when a client is put in server.
	 *
	 * @param client		Index of the client.
	 */
	virtual void OnClientPutInServer(int client);

	/**
	 * @brief Called when a client is disconnecting (not fully disconnected yet).
	 *
	 * @param client		Index of the client.
	 */
	virtual void OnClientDisconnecting(int client);

	/**
	 * @brief Called when a client has fully disconnected.
	 *
	 * @param client		Index of the client.
	 */
	virtual void OnClientDisconnected(int client)
	{
	}

	/**
	 * @brief Called when a client has received authorization.
	 *
	 * @param client		Index of the client.
	 * @param authstring	Client Steam2 id, if available, else engine auth id.
	 */
	virtual void OnClientAuthorized(int client, const char* authstring)
	{
	}

	/**
	 * @brief Called when the server is activated.
	 */
	virtual void OnServerActivated(int max_clients)
	{
	}

	/**
	 * @brief Called once a client is authorized and fully in-game, but
	 * before admin checks are done.  This can be used to override the
	 * default admin checks for a client.
	 *
	 * By default, this function allows the authentication process to
	 * continue as normal.  If you need to delay the cache searching
	 * process in order to get asynchronous data, then return false here.
	 *
	 * If you return false, you must call IPlayerManager::NotifyPostAdminCheck
	 * for the same client, or else the OnClientPostAdminCheck callback will
	 * never be called.
	 *
	 * @param client		Client index.
	 * @return				True to continue normally, false to override
	 *						the authentication process.
	 */
	virtual bool OnClientPreAdminCheck(int client)
	{
		return true;
	}

	/**
	 * @brief Called once a client is authorized and fully in-game, and
	 * after all post-connection authorizations have been passed.  If the
	 * client does not have an AdminId by this stage, it means that no
	 * admin entry was in the cache that matched, and the user could not
	 * be authenticated as an admin.
	 *
	 * @param client		Client index.
	 */
	virtual void OnClientPostAdminCheck(int client)
	{
	}

	/**
	* @brief Notifies the extension that the maxplayers value has changed
	*
	* @param newvalue			New maxplayers value.
	*/
	virtual void OnMaxPlayersChanged(int newvalue)
	{
	}

	/**
	* @brief Notifies the extension that a clients settings changed
	*
	* @param client			Client index.
	*/
	virtual void OnClientSettingsChanged(int client)
	{
	}

	void Hook_GameFrame(bool simulating); // Post hook

#if SOURCE_ENGINE <= SE_DARKMESSIAH
	// Source 2006
	void Hook_ClientCommand_OldEngine(edict_t* pEntity);
#else
	// Source 2007 and newer
	void Hook_ClientCommand(edict_t* pEntity, const CCommand& args);
#endif // SOURCE_ENGINE <= SE_DARKMESSIAH


#if defined(EXT_DEBUG) && SOURCE_ENGINE >= SE_EYE
	void Hook_ClientCommandKeyValues(edict_t* pEntity, KeyValues* pKeyValues);
#endif // EXT_DEBUG


	bool ShouldCallRunPlayerCommand() const { return !m_hookruncmd; }
	// True if we should SourceHook CBasePlayer::PlayerRunCommand
	bool ShouldHookRunPlayerCommand() const { return m_hookruncmd; }
	SourceMod::IGameConfig* GetExtensionGameData() { return m_cfg_navbot; }
	SourceMod::IGameConfig* GetSDKHooksGameData() { return m_cfg_sdkhooks; }
	SourceMod::IGameConfig* GetSDKToolsGameData() { return m_cfg_sdktools; }

	std::vector<sp_nativeinfo_t>& GetNativesVector() { return m_natives; }

private:
	bool m_hookruncmd;
	bool m_botsAreSupported; // true if it's possible to add bots
	SourceMod::IGameConfig* m_cfg_navbot;
	SourceMod::IGameConfig* m_cfg_sdktools;
	SourceMod::IGameConfig* m_cfg_sdkhooks;
	std::vector<sp_nativeinfo_t> m_natives;
};

extern NavBotExt* extension;
extern CGlobalVars* gpGlobals;
extern IVDebugOverlay* debugoverlay;
extern IServerGameDLL* servergamedll;
extern IServerGameEnts* servergameents;
extern IEngineTrace* enginetrace;
extern IEngineSound* enginesound;
extern IPlayerInfoManager* playerinfomanager;
extern IServerGameClients* gameclients;
extern IGameEventManager2* gameeventmanager;
extern IPhysicsSurfaceProps* physprops;
extern IVModelInfo* modelinfo;
extern IMDLCache* imdlcache;
extern IFileSystem* filesystem;
extern ICvar* icvar;
#if SOURCE_ENGINE <= SE_DARKMESSIAH
extern ICvar* g_pCVar;
#endif // SOURCE_ENGINE <= SE_DARKMESSIAH
extern IServerTools* servertools;
extern IServerPluginHelpers* serverpluginhelpers;
extern IStaticPropMgrServer* staticpropmgr;
extern ISpatialPartition* partition;
extern CBaseEntityList* g_EntList;

extern IBinTools* g_pBinTools;
extern ISDKTools* g_pSDKTools;
extern ISDKHooks* g_pSDKHooks;

extern IBotManager* botmanager;

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
