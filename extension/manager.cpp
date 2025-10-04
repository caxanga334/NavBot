#include NAVBOT_PCH_FILE
#include <vector>
#include <stdexcept>
#include <filesystem>
#include <string>
#include <fstream>
#include <memory>
#include <algorithm>
#include <cstring>

#include "extension.h"

#include <bot/basebot.h>
#include <bot/bot_debug_shared.h>

#include <engine/ivdebugoverlay.h>
#include <mods/basemod.h>
#include <extplayer.h>
#include <util/helpers.h>
#include <util/librandom.h>
#include "manager.h"

#include <navmesh/nav_mesh.h>

#ifdef EXT_DEBUG
#include <sdkports/debugoverlay_shared.h>
#include <navmesh/nav.h>
#include <navmesh/nav_area.h>
#include <navmesh/nav_pathfind.h>
#include <bot/interfaces/path/basepath.h>
#include <entities/baseentity.h>
#endif // EXT_DEBUG

#if SOURCE_ENGINE == SE_TF2
#include <mods/tf2/teamfortress2mod.h>
#endif // SOURCE_ENGINE == SE_TF2

#if SOURCE_ENGINE == SE_DODS
#include <mods/dods/dayofdefeatsourcemod.h>
#endif // SOURCE_ENGINE == SE_DODS

#if SOURCE_ENGINE == SE_BMS
#include <mods/blackmesa/blackmesadm_mod.h>
#endif // SOURCE_ENGINE == SE_BMS

#if SOURCE_ENGINE == SE_HL2DM
#include <mods/hl2dm/hl2dm_mod.h>
#endif // SOURCE_ENGINE == SE_HL2DM

#if SOURCE_ENGINE == SE_EPISODEONE
#include <sdkports/sdk_convarref_ep1.h>
#endif // SOURCE_ENGINE == SE_EPISODEONE

#include <bot/pluginbot/pluginbot.h>

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

#undef max // valve mathlib conflict fix
#undef min
#undef clamp

constexpr auto BOT_QUOTA_UPDATE_INTERVAL = 2.0f;

static ConVar sm_navbot_quota_mode("sm_navbot_quota_mode", "normal", FCVAR_GAMEDLL, "NavBot bot quota mode. \n'normal' = Keep N number of bots in the game.\n'fill' = Fill to N bots, remove to make space for human players", CExtManager::OnQuotaModeCvarChanged);
static ConVar sm_navbot_quota_quantity("sm_navbot_quota_quantity", "0", FCVAR_GAMEDLL, "Number of bots to add.", CExtManager::OnQuotaTargetCvarChanged);
static ConVar sm_navbot_bot_name_prefix("sm_navbot_bot_name_prefix", "", FCVAR_GAMEDLL, "Prefix to add to bot names.");

// Engine branches that supports engine->CreateFakeClientEx
#if SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS
static ConVar sm_navbot_hide_bots("sm_navbot_hide_bots", "0", FCVAR_GAMEDLL, "If enabled, bots will be hidden from the server browser.");
#endif

CExtManager::CExtManager()
{
	m_bots.reserve(128); // 128 should be good for most mods
	m_players.reserve(128);
	m_botnames.reserve(256); // reserve space for 256 bot names, vector size will increase if needed
	m_nextbotname = 0U;
	m_botdebugmode = BOTDEBUG_NONE;
	m_quotamode = BotQuotaMode::QUOTA_FIXED;
	m_quotatarget = 0;
	m_quotaupdatetime = TIME_TO_TICKS(BOT_QUOTA_UPDATE_INTERVAL);
	m_iscreatingbot = false;
	m_callModUpdateTimer.Invalidate();
	m_pawnmemory = std::make_unique<CSourcePawnMemoryManager>();
	m_allowbots = true;
#ifndef NO_SOURCEPAWN_API
	m_prebotaddforward = nullptr;
	m_postbotaddforward = nullptr;
	m_prepluginbotaddforward = nullptr;
	m_postpluginbotaddforward = nullptr;
	m_preremoverandombot = nullptr;
	m_botquotaisclientignored = nullptr;
#endif // !NO_SOURCEPAWN_API

}

CExtManager::~CExtManager()
{
	m_bots.clear();
	m_players.clear();
#ifndef NO_SOURCEPAWN_API
	forwards->ReleaseForward(m_prebotaddforward);
	forwards->ReleaseForward(m_postbotaddforward);
	forwards->ReleaseForward(m_prepluginbotaddforward);
	forwards->ReleaseForward(m_postpluginbotaddforward);
	forwards->ReleaseForward(m_preremoverandombot);
	forwards->ReleaseForward(m_botquotaisclientignored);
#endif // !NO_SOURCEPAWN_API
}

void CExtManager::OnAllLoaded()
{
#ifndef NO_SOURCEPAWN_API
	m_prebotaddforward = forwards->CreateForward("OnPreNavBotAdd", ET_Event, 0, nullptr);
	m_postbotaddforward = forwards->CreateForward("OnNavBotAdded", ET_Ignore, 1, nullptr, SourceMod::ParamType::Param_Cell);
	m_prepluginbotaddforward = forwards->CreateForward("OnPrePluginBotAdd", ET_Event, 1, nullptr, SourceMod::ParamType::Param_Cell);
	m_postpluginbotaddforward = forwards->CreateForward("OnPluginBotAdded", ET_Ignore, 1, nullptr, SourceMod::ParamType::Param_Cell);
	m_preremoverandombot = forwards->CreateForward("OnPreKickRandomBot", ET_Event, 1, nullptr, SourceMod::ParamType::Param_CellByRef);
	m_botquotaisclientignored = forwards->CreateForward("OnUpdateBotQuotaIsClientIgnored", ET_Single, 1, nullptr, SourceMod::ParamType::Param_Cell);
#endif // !NO_SOURCEPAWN_API

	AllocateMod();
	LoadBotNames();

	constexpr std::size_t MAX_LEN = 256U;
	char error[MAX_LEN];

	SourceMod::IGameConfig* gameconf = nullptr;

	if (gameconfs->LoadGameConfigFile("navbot.games", &gameconf, error, MAX_LEN))
	{
		const char* botmethod = gameconf->GetKeyValue("CreateBotMethod");

		if (botmethod)
		{
			if (std::strcmp(botmethod, "engine1") == 0)
			{
				CExtManager::s_botcreatemethod = BotCreateMethod::CREATEFAKECLIENT;
			}
			else if (std::strcmp(botmethod, "engine2") == 0)
			{
				CExtManager::s_botcreatemethod = BotCreateMethod::CREATEFAKECLIENTEX;
			}
			else if (std::strcmp(botmethod, "botmanager") == 0)
			{
				CExtManager::s_botcreatemethod = BotCreateMethod::BOTMANAGER;
			}
		}

		const char* value = gameconf->GetKeyValue("FixUpFlags");
		
		if (value)
		{
			CExtManager::s_fixupbotflags = UtilHelpers::StringToBoolean(value);
		}

		gameconfs->CloseGameConfigFile(gameconf);
	}

	smutils->LogMessage(myself, "Extension fully loaded. Source Engine '%s'. Detected Mod: '%s'", UtilHelpers::GetEngineBranchName(), m_mod->GetModName());
}

void CExtManager::Frame()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CExtManager::Frame", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (--m_quotaupdatetime <= 0)
	{
		m_quotaupdatetime = TIME_TO_TICKS(BOT_QUOTA_UPDATE_INTERVAL);

		UpdateBotQuota();
	}

	/*
	* This is now called by the CBasePlayer::PhysicsSimulate() hook
	for (auto& botptr : m_bots)
	{
		auto bot = botptr.get();
		bot->PlayerThink();
	}
	*/

	m_mod->Frame();

	if (m_callModUpdateTimer.IsElapsed())
	{
		m_callModUpdateTimer.Start(MOD_UPDATE_INTERVAL);
		m_mod->Update();
	}

	for (auto& player : m_players)
	{
		player->PlayerThink(); // Call player think, updates last known nav area for non NavBot clients
	}
}

void CExtManager::OnClientPutInServer(int client)
{
	edict_t* edict = gamehelpers->EdictOfIndex(client);

	if (edict == nullptr)
	{
		smutils->LogError(myself, "CExtManager::OnClientPutInServer failed to convert client index to edict pointer!");
		return;
	}

	if (m_iscreatingbot)
	{
		smutils->LogMessage(myself, "Adding NavBot to the game. #%i<%p>", client, edict);
		auto& bot = m_bots.emplace_back(m_mod->AllocateBot(edict));
		bot->PostConstruct();
	}
	else
	{
		// not a NavBot, register as a player
		m_players.emplace_back(new CBaseExtPlayer(edict));
	}
	
	auto gp = playerhelpers->GetGamePlayer(client);

	if (gp->IsSourceTV() || gp->IsReplay())
		return; // Don't care about sourcetv bots

#ifdef EXT_DEBUG
	auto auth = gp->GetAuthString(true);

	if (auth == nullptr)
	{
		auth = "NULL";
	}

	smutils->LogMessage(myself, "OnClientPutInServer -- %i %p '%s'", client, edict, auth);
#endif // EXT_DEBUG
}

void CExtManager::OnClientDisconnect(int client)
{
	auto gp = playerhelpers->GetGamePlayer(client);

	if (gp->IsSourceTV() || gp->IsReplay())
		return; // Don't care about sourcetv bots

	m_bots.erase(std::remove_if(m_bots.begin(), m_bots.end(), [&client](const std::unique_ptr<CBaseBot>& object) {
		if (object->GetIndex() == client)
		{
			return true;
		}

		return false;
	}), m_bots.end());

	m_players.erase(std::remove_if(m_players.begin(), m_players.end(), [&client](const std::unique_ptr<CBaseExtPlayer>& object) {
		if (object->GetIndex() == client)
		{
			return true;
		}

		return false;
	}), m_players.end());
}

void CExtManager::OnMapStart()
{
	TheNavMesh->OnMapStart();
	m_mod->OnMapStart();

	if (m_botnames.size() != 0)
	{
		// get a new index on every map load
		m_nextbotname = randomgen->GetRandomInt<size_t>(0U, m_botnames.size() - 1U);
	}

	auto mode = sm_navbot_quota_mode.GetString();

	if (strncasecmp(mode, "normal", 6) == 0)
	{
		SetBotQuotaMode(BotQuotaMode::QUOTA_FIXED);
	}
	else if (strncasecmp(mode, "fill", 4) == 0)
	{
		SetBotQuotaMode(BotQuotaMode::QUOTA_FILL);
	}
	else
	{
		SetBotQuotaMode(BotQuotaMode::QUOTA_FIXED);
		smutils->LogError(myself, "Unknown bot quota mode \"%s\"!", mode);
	}

	int target = sm_navbot_quota_quantity.GetInt();
	target = std::clamp(target, 0, gpGlobals->maxClients - 1); // limit max bots to server maxplayers - 1
	SetBotQuotaTarget(target);

	ConVarRef sv_quota_stringcmdspersecond("sv_quota_stringcmdspersecond");

	if (sv_quota_stringcmdspersecond.IsValid())
	{
		CBaseBot::m_maxStringCommandsPerSecond = std::max(sv_quota_stringcmdspersecond.GetInt(), 0);
	}
	else
	{
		// either failed to find the convar or it does not exists
		CBaseBot::m_maxStringCommandsPerSecond = 20;
	}

	ConVarRef sv_gravity("sv_gravity");

	CExtManager::s_sv_gravity = sv_gravity.GetFloat();
	m_callModUpdateTimer.Start(MOD_UPDATE_AFTER_MAP_LOAD);
	m_quotaupdatetime = TIME_TO_TICKS(10.0f);
}

void CExtManager::OnMapEnd()
{
	TheNavMesh->OnMapEnd();
	m_mod->OnMapEnd();
}

// Detect current mod and initializes it
void CExtManager::AllocateMod()
{
	// Don't check game folder name unless the SDK supports multiple mods (SDK2013, EP2 (Orange box), EP1 (Original))

#if SOURCE_ENGINE == SE_TF2
	m_mod = std::make_unique<CTeamFortress2Mod>();
#elif SOURCE_ENGINE == SE_DODS
	m_mod = std::make_unique<CDayOfDefeatSourceMod>();
#elif SOURCE_ENGINE == SE_BMS
	m_mod = std::make_unique<CBlackMesaDeathmatchMod>();
#elif SOURCE_ENGINE == SE_HL2DM
	{
		const char* gamefolder = smutils->GetGameFolderName();

		// Mods based on the new version of the Source 2013 SDK are detected as HL2DM engine.
		// See: https://github.com/alliedmodders/metamod-source/pull/210
		if (strncasecmp(gamefolder, "hl2mp", 5) == 0)
		{
			m_mod = std::make_unique<CHalfLife2DeathMatchMod>();
		}
		else
		{
			m_mod = std::make_unique<CBaseMod>();
		}
	}
#else
	m_mod = std::make_unique<CBaseMod>();
#endif // SOURCE_ENGINE == SE_TF2

	m_mod->PostCreation();
}

CBaseMod* CExtManager::GetMod()
{
	return m_mod.get();
}

CBaseBot* CExtManager::GetBotByIndex(int index) const
{
	for (auto& botptr : m_bots)
	{
		auto bot = botptr.get();
		if (bot->GetIndex() == index)
		{
			return bot;
		}
	}

	return nullptr;
}

CBaseExtPlayer* CExtManager::GetPlayerByIndex(int index) const
{
	CBaseBot* bot = GetBotByIndex(index);

	if (bot)
	{
		// CBaseExtPlayer is a base class of CBaseBot
		return static_cast<CBaseExtPlayer*>(bot);
	}

	for (auto& ptr : m_players)
	{
		if (ptr->GetIndex() == index)
		{
			return ptr.get();
		}
	}

#ifdef EXT_DEBUG
	IGamePlayer* player = playerhelpers->GetGamePlayer(index);

	if (player && player->IsInGame())
	{
		smutils->LogError(myself, "CExtManager::GetPlayerByIndex MISSED PLAYER OF INDEX %i!", index);
	}
#endif // EXT_DEBUG

	return nullptr;
}

CBaseBot* CExtManager::GetBotFromEntity(CBaseEntity* entity) const
{
	if (entity == nullptr) { return nullptr; }

	for (auto& botptr : m_bots)
	{
		auto bot = botptr.get();
		if (bot->GetEntity() == entity)
		{
			return bot;
		}
	}

	return nullptr;
}

CBaseExtPlayer* CExtManager::GetPlayerOfEntity(CBaseEntity* entity) const
{
	if (entity == nullptr) { return nullptr; }

	for (auto& botptr : m_bots)
	{
		auto bot = botptr.get();
		if (bot->GetEntity() == entity)
		{
			return static_cast<CBaseExtPlayer*>(bot);
		}
	}

	for (auto& player : m_players)
	{
		if (player->GetEntity() == entity)
		{
			return player.get();
		}
	}

	return nullptr;
}

CBaseExtPlayer* CExtManager::GetPlayerOfEdict(edict_t* edict) const
{
	if (edict == nullptr) { return nullptr; }

	for (auto& botptr : m_bots)
	{
		auto bot = botptr.get();
		if (bot->GetEdict() == edict)
		{
			return static_cast<CBaseExtPlayer*>(bot);
		}
	}

	for (auto& player : m_players)
	{
		if (player->GetEdict() == edict)
		{
			return player.get();
		}
	}

	return nullptr;
}

CBaseExtPlayer* CExtManager::GetListenServerHost() const
{
	if (engine->IsDedicatedServer())
	{
		return nullptr;
	}

	for (auto& player : m_players)
	{
		if (player->GetIndex() == 1)
		{
			return player.get();
		}
	}

	return nullptr;
}

CBaseBot* CExtManager::FindBotByName(const char* name) const
{
	for (auto& botptr : m_bots)
	{
		const char* other = botptr->GetClientName();

		if (V_strstr(name, other) != nullptr)
		{
			return botptr.get();
		}
	}

	return nullptr;
}

bool CExtManager::IsNavBot(const int client) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CExtManager::IsNavBot", "NavBot");
#endif // EXT_VPROF_ENABLED

	for (auto& botptr : m_bots)
	{
		if (botptr.get()->GetIndex() == client)
		{
			return true;
		}
	}

	return false;
}

void CExtManager::AddBot(std::string* newbotname, edict_t** newbotedict)
{
	if (!m_allowbots)
	{
		return;
	}

	if (TheNavMesh->IsDangerousForBots())
	{
		META_CONPRINT("The navigation mesh was edited, bots cannot be added until the map is reloaded! \n");
		return;
	}

	cell_t result = static_cast<cell_t>(SourceMod::ResultType::Pl_Continue);

#ifndef NO_SOURCEPAWN_API
	m_prebotaddforward->Execute(&result);

	if (result > static_cast<cell_t>(SourceMod::ResultType::Pl_Continue))
	{
		return;
	}
#endif // !NO_SOURCEPAWN_API

	char defaultbotname[32]{};
	const char* name = nullptr;

	if (newbotname != nullptr)
	{
		name = newbotname->c_str();
	}
	else
	{
		if (m_botnames.empty())
		{
			int n = 0;

			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				SourceMod::IGamePlayer* player = playerhelpers->GetGamePlayer(i);

				if (player && player->IsInGame())
				{
					continue;
				}

				n = i;
				break;
			}

			ke::SafeSprintf(defaultbotname, sizeof(defaultbotname), "Bot %i", n);
			name = defaultbotname;
		}
		else
		{
			auto& botname = m_botnames[m_nextbotname];
			m_nextbotname++;

			if (m_nextbotname >= m_botnames.size())
			{
				m_nextbotname = 0; // go back to start
			}

			name = botname.c_str();
		}
	}

	char finalname[MAX_PLAYER_NAME_LENGTH + 1]{};
	const char* prefix = sm_navbot_bot_name_prefix.GetString();

	// add prefix if set
	if (prefix && prefix[0] && std::strlen(prefix) > 0)
	{
		ke::SafeSprintf(finalname, sizeof(finalname), "%s%s", prefix, name);
	}
	else
	{
		// just copy the name
		ke::SafeStrcpy(finalname, sizeof(finalname), name);
	}

	// Tell the bot manager to create a new bot. Now that we are using SourceHooks, we need to catch the bot on 'OnClientPutInServer'.
	m_iscreatingbot = true;
	edict_t* edict = nullptr;

	switch (CExtManager::s_botcreatemethod)
	{
	case BotCreateMethod::CREATEFAKECLIENT:
		edict = engine->CreateFakeClient(finalname);
		break;
	case BotCreateMethod::CREATEFAKECLIENTEX:
#if SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS
		edict = engine->CreateFakeClientEx(finalname, !sm_navbot_hide_bots.GetBool());
#else
		edict = engine->CreateFakeClient(finalname);
#endif // SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS
		break;
	case BotCreateMethod::BOTMANAGER:
		[[fallthrough]];
	default:
		edict = botmanager->CreateBot(finalname);
		break;
	}

	m_iscreatingbot = false;

	if (edict == nullptr)
	{
		smutils->LogError(myself, "AddBot failed: NULL edict when creating bot!");

		if (newbotedict != nullptr)
		{
			*newbotedict = nullptr;
		}

		return;
	}

	auto bot = GetBotByIndex(gamehelpers->IndexOfEdict(edict));

	bot->PostAdd();

	// The base bot doesn't create these interfaces until something uses them, so we call these functions here to do that.
	bot->GetControlInterface();
	bot->GetMovementInterface();
	bot->GetSensorInterface();
	bot->GetBehaviorInterface();
	bot->GetInventoryInterface();
	bot->GetSquadInterface();
	bot->GetCombatInterface();

	if (newbotedict != nullptr)
	{
		*newbotedict = edict;
	}

#ifndef NO_SOURCEPAWN_API
	m_postbotaddforward->PushCell(gamehelpers->IndexOfEdict(edict));
	m_postbotaddforward->Execute();
#endif // !NO_SOURCEPAWN_API

	smutils->LogMessage(myself, "NavBot added to the game.");
}

CBaseBot* CExtManager::AttachBotInstanceToEntity(edict_t* entity)
{
#ifndef NO_SOURCEPAWN_API
	if (!m_allowbots)
	{
		return nullptr;
	}

	cell_t result = static_cast<cell_t>(SourceMod::ResultType::Pl_Continue);

	m_prepluginbotaddforward->PushCell(gamehelpers->IndexOfEdict(entity));
	m_prepluginbotaddforward->Execute(&result);

	if (result > static_cast<cell_t>(SourceMod::ResultType::Pl_Continue))
	{
		return nullptr;
	}

	CBaseBot* newBot = static_cast<CBaseBot*>(new CPluginBot(entity));
	newBot->PostAdd();
	m_bots.emplace_back(newBot);

	m_postpluginbotaddforward->PushCell(gamehelpers->IndexOfEdict(entity));
	m_postpluginbotaddforward->Execute();

	return newBot;
#else
	return nullptr;
#endif // !NO_SOURCEPAWN_API
}

void CExtManager::RemoveRandomBot(const char* message)
{
	if (m_bots.empty())
	{
		return;
	}

#ifndef NO_SOURCEPAWN_API
	cell_t plugin_target = 0;
	cell_t result = static_cast<cell_t>(SourceMod::ResultType::Pl_Continue);

	m_preremoverandombot->PushCellByRef(&plugin_target);
	m_preremoverandombot->Execute(&result);

	if (result >= static_cast<cell_t>(SourceMod::ResultType::Pl_Changed))
	{
		if (result == static_cast<cell_t>(SourceMod::ResultType::Pl_Stop))
		{
			return;
		}

		CBaseBot* bot = GetBotByIndex(static_cast<int>(plugin_target));

		if (!bot)
		{
			smutils->LogError(myself, "Forward error: OnPreKickRandomBot! Given client index %i is not a NavBot instance.", static_cast<int>(plugin_target));
			return;
		}

		IGamePlayer* gp = playerhelpers->GetGamePlayer(bot->GetIndex());
		gp->Kick(message);
		return;
	}
#endif // !NO_SOURCEPAWN_API

	auto& botptr = m_bots[randomgen->GetRandomInt<size_t>(0U, m_bots.size() - 1)];
	auto player = playerhelpers->GetGamePlayer(botptr->GetIndex());
	player->Kick(message);
}

void CExtManager::RemoveAllBots(const char* message) const
{
	for (int client = 1; client <= gpGlobals->maxClients; client++)
	{
		IGamePlayer* player = playerhelpers->GetGamePlayer(client);

		if (player && IsNavBot(client))
		{
			player->Kick(message);
		}
	}
}

void CExtManager::LoadBotNames()
{
	char path[PLATFORM_MAX_PATH]{};

	m_botnames.clear(); // clear vector if we're reloading the name list
	smutils->BuildPath(SourceMod::PathType::Path_SM, path, sizeof(path), "configs/navbot/bot_names.cfg");

	if (!std::filesystem::exists(path))
	{
		return;
	}

	std::fstream file;
	std::string line;

	constexpr size_t MAX_BOT_NAME_LENGTH = 31U;
	file.open(path, std::fstream::in);
	line.reserve(64);

	while (std::getline(file, line))
	{
		if (line.find("//", 0, 2) != std::string::npos)
		{
			continue; // comment line, ignore
		}

		if (std::isspace(line[0]) != 0)
		{
			continue; // ignore lines that start with space
		}

		// we don't want these
		line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
		line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());

		if (line.length() == 0)
		{
			continue;
		}

		if (line.length() > MAX_BOT_NAME_LENGTH)
		{
			smutils->LogError(myself, "Bot name \"%s\" over character limit! skipping...", line.c_str());
			continue;
		}

		m_botnames.emplace_back(line);
	}

	// shuffle the names
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(m_botnames.begin(), m_botnames.end(), g);

	rootconsole->ConsolePrint("[NavBot] Bot name list loaded with %i names.", m_botnames.size());
}

void CExtManager::UpdateBotQuota()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CExtManager::UpdateBotQuota", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (!m_allowbots || m_quotatarget == 0 || !TheNavMesh->IsLoaded())
		return;

	int humans = 0;
	int navbots = 0;
	int otherbots = 0;
	const CBaseMod* mod = m_mod.get();
	std::array<bool, ABSOLUTE_PLAYER_LIMIT + 1> isnavbot;
	std::fill(std::begin(isnavbot), std::end(isnavbot), false);

	for (auto& ptr : m_bots)
	{
		isnavbot[ptr->GetIndex()] = true;
	}
	
	auto func = [&mod, &humans, &navbots, &otherbots, &isnavbot](int client, edict_t* entity, SourceMod::IGamePlayer* player) -> void {
		if (player->IsInGame())
		{
#ifndef NO_SOURCEPAWN_API
			if (extmanager->SMAPIBotQuotaIsClientIgnored(client))
			{
				return;
			}
#endif

			if (mod->BotQuotaIsClientIgnored(client, entity, player))
			{
				return;
			}

			// Ignore STV/Replay bots.
			if (player->IsSourceTV() || player->IsReplay())
			{
				return;
			}

			if (player->IsFakeClient())
			{
				if (isnavbot[client])
				{
					++navbots;
				}
				else
				{
					++otherbots;
				}
			}
			else
			{
				++humans;
			}
		}
	};

	UtilHelpers::ForEachPlayer(func);

	if (humans == 0) // server is empty of humans
	{
		if (navbots > 0)
		{
			// Remove all NavBots to save server CPU
			// Some games will do this automatically for us
			RemoveAllBots("NavBot: Server is Empty. Removing Bots.");
		}

		return;
	}

#ifdef EXT_DEBUG
	rootconsole->ConsolePrint("[NavBot] Updating Bot Quota \n   %i humans %i navbots %i otherbots ", humans, navbots, otherbots);
#endif // EXT_DEBUG

	switch (m_quotamode)
	{

	case CExtManager::BotQuotaMode::QUOTA_FILL:
	{
		int target = m_quotatarget - (humans + otherbots);

		if (target == 0 && navbots == 0)
			return;

		if (navbots < target)
		{
			AddBot();
		}
		else if (navbots > target)
		{
			RemoveRandomBot("Nav Bot: Kicked by bot quota manager.");
		}

		break;
	}
	case CExtManager::BotQuotaMode::QUOTA_FIXED:
	default:
		if (navbots < m_quotatarget) // number of bots is below desired quantity
		{
			AddBot();
		}
		else if (navbots > m_quotatarget) // number of bots is above desired quantity
		{
			RemoveRandomBot("Nav Bot: Kicked by bot quota manager.");
		}

		break;
	}
}

#if SOURCE_ENGINE > SE_EPISODEONE
void CExtManager::OnQuotaModeCvarChanged(IConVar* var, const char* pOldValue, float flOldValue)
#else
void CExtManager::OnQuotaModeCvarChanged(ConVar* var, char const* pOldString)
#endif
{
	auto mode = sm_navbot_quota_mode.GetString();

	if (strncasecmp(mode, "normal", 6) == 0)
	{
		extmanager->SetBotQuotaMode(BotQuotaMode::QUOTA_FIXED);
	}
	else if (strncasecmp(mode, "fill", 4) == 0)
	{
		extmanager->SetBotQuotaMode(BotQuotaMode::QUOTA_FILL);
	}
	else
	{
		extmanager->SetBotQuotaMode(BotQuotaMode::QUOTA_FIXED);
		smutils->LogError(myself, "Unknown bot quota mode \"%s\"!", mode);
	}

	if (!TheNavMesh->IsLoaded())
	{
		Warning("[NAVBOT] Nav mesh not loaded, bot quota unavailable.\n");
	}
}

#if SOURCE_ENGINE > SE_EPISODEONE
void CExtManager::OnQuotaTargetCvarChanged(IConVar* var, const char* pOldValue, float flOldValue)
#else
void CExtManager::OnQuotaTargetCvarChanged(ConVar* var, char const* pOldString)
#endif
{
	int target = sm_navbot_quota_quantity.GetInt();
	target = std::clamp(target, 0, gpGlobals->maxClients - 1); // limit max bots to server maxplayers - 1
	extmanager->SetBotQuotaTarget(target);
}

void CExtManager::OnClientCommand(edict_t* pEdict, SourceMod::IGamePlayer* player, const CCommand& args)
{
	m_mod->OnClientCommand(pEdict, player, args);
}

int CExtManager::AutoComplete_BotNames(const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	if (V_strlen(partial) >= COMMAND_COMPLETION_ITEM_LENGTH)
	{
		return 0;
	}

	char cmd[COMMAND_COMPLETION_ITEM_LENGTH + 2];
	V_strncpy(cmd, partial, sizeof(cmd));

	// skip to start of argument
	char* partialArg = V_strrchr(cmd, ' ');
	if (partialArg == NULL)
	{
		return 0;
	}

	// chop command from partial argument
	*partialArg = '\000';
	++partialArg;

	int partialArgLength = V_strlen(partialArg);

	int count = 0;

	for (auto& botptr : extmanager->m_bots)
	{
		const char* name = botptr->GetClientName();

		if (count >= COMMAND_COMPLETION_MAXITEMS)
		{
			break;
		}

		if (V_strnicmp(name, partialArg, partialArgLength) == 0)
		{
			V_snprintf(commands[count++], COMMAND_COMPLETION_ITEM_LENGTH, "%s \"%s\"", cmd, name);
		}
	}

	return count;
}

#ifndef NO_SOURCEPAWN_API
bool CExtManager::SMAPIBotQuotaIsClientIgnored(int client)
{
	cell_t result = 0;
	m_botquotaisclientignored->PushCell(static_cast<cell_t>(client));
	m_botquotaisclientignored->Execute(&result);

	return result != 0;
}
#endif // !NO_SOURCEPAWN_API

CON_COMMAND(sm_navbot_reload_name_list, "Reloads the bot name list")
{
	extmanager->LoadBotNames();
}

static std::string s_debugoptionsnames[] = {
	{ "STOPALL" },
	{ "SENSOR" },
	{ "TASKS" },
	{ "LOOK" },
	{ "PATH" },
	{ "EVENTS" },
	{ "MOVEMENT" },
	{ "ERRORS" },
	{ "MISC" },
	{ "COMBAT" }
};

static int SMNavBotDebugCommand_AutoComplete(const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	if (V_strlen(partial) >= COMMAND_COMPLETION_ITEM_LENGTH)
	{
		return 0;
	}

	char cmd[COMMAND_COMPLETION_ITEM_LENGTH + 2];
	V_strncpy(cmd, partial, sizeof(cmd));

	// skip to start of argument
	char* partialArg = V_strrchr(cmd, ' ');
	if (partialArg == NULL)
	{
		return 0;
	}

	// chop command from partial argument
	*partialArg = '\000';
	++partialArg;

	int partialArgLength = V_strlen(partialArg);

	int count = 0;

	for (auto& optionname : s_debugoptionsnames)
	{
		if (count >= COMMAND_COMPLETION_MAXITEMS)
		{
			break;
		}

		if (V_strnicmp(optionname.c_str(), partialArg, partialArgLength) == 0)
		{
			V_snprintf(commands[count++], COMMAND_COMPLETION_ITEM_LENGTH, "%s %s", cmd, optionname.c_str());
		}
	}

	return count;
}

CON_COMMAND_F_COMPLETION(sm_navbot_debug, "Toggles between debug modes", FCVAR_CHEAT, SMNavBotDebugCommand_AutoComplete)
{
	DECLARE_COMMAND_ARGS;

	if (args.ArgC() <= 1)
	{
		rootconsole->ConsolePrint("Available debug options: STOPALL, SENSOR, TASKS, LOOK, PATH, EVENTS, MOVEMENT, ERRORS, MISC, COMBAT");
		rootconsole->ConsolePrint("Usage: sm_navbot_debug <OPTION1> <OPTION2> ...");
		return;
	}
	
	for (int i = 1; i < args.ArgC(); i++)
	{
		auto option = args.Arg(i);

		if (strncasecmp(option, "STOPALL", 6) == 0)
		{
			extmanager->StopAllDebugging();
			rootconsole->ConsolePrint("Stopped debugging");
			return;
		}

		if (strncasecmp(option, "SENSOR", 6) == 0)
		{
			extmanager->ToggleDebugOption(BOTDEBUG_SENSOR);
			rootconsole->ConsolePrint("Toggle Debugging Bot Sensor Interface");
		}
		else if (strncasecmp(option, "TASKS", 5) == 0)
		{
			extmanager->ToggleDebugOption(BOTDEBUG_TASKS);
			rootconsole->ConsolePrint("Toggle Debugging Bot Tasks");
		}
		else if (strncasecmp(option, "LOOK", 4) == 0)
		{
			extmanager->ToggleDebugOption(BOTDEBUG_LOOK);
			rootconsole->ConsolePrint("Toggle Debugging Bot Look");
		}
		else if (strncasecmp(option, "PATH", 4) == 0)
		{
			extmanager->ToggleDebugOption(BOTDEBUG_PATH);
			rootconsole->ConsolePrint("Toggle Debugging Bot Path");
		}
		else if (strncasecmp(option, "EVENTS", 6) == 0)
		{
			extmanager->ToggleDebugOption(BOTDEBUG_EVENTS);
			rootconsole->ConsolePrint("Toggle Debugging Bot Events");
		}
		else if (strncasecmp(option, "MOVEMENT", 8) == 0)
		{
			extmanager->ToggleDebugOption(BOTDEBUG_MOVEMENT);
			rootconsole->ConsolePrint("Toggle Debugging Bot Movement");
		}
		else if (strncasecmp(option, "ERRORS", 6) == 0)
		{
			extmanager->ToggleDebugOption(BOTDEBUG_ERRORS);
			rootconsole->ConsolePrint("Toggle Debugging Bot Error");
		}
		else if (strncasecmp(option, "SQUADS", 6) == 0)
		{
			extmanager->ToggleDebugOption(BOTDEBUG_SQUADS);
			rootconsole->ConsolePrint("Toggle Debugging Bot Squads");
		}
		else if (strncasecmp(option, "MISC", 4) == 0)
		{
			extmanager->ToggleDebugOption(BOTDEBUG_MISC);
			rootconsole->ConsolePrint("Toggle Debugging Bot Miscellaneous functions");
		}
		else if (strncasecmp(option, "COMBAT", 6) == 0)
		{
			extmanager->ToggleDebugOption(BOTDEBUG_COMBAT);
			rootconsole->ConsolePrint("Toggle Debugging Bot Combat functions");
		}
		else
		{
			rootconsole->ConsolePrint("Unknown option \"%s\".", option);
			rootconsole->ConsolePrint("Available debug options: STOPALL, SENSOR, TASKS, LOOK, PATH, EVENTS, MOVEMENT, ERRORS, SQUADS");
			rootconsole->ConsolePrint("Usage: sm_navbot_debug <OPTION1> <OPTION2> ...");
		}
	}
}

