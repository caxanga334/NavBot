#include <vector>
#include <stdexcept>
#include <filesystem>
#include <string>
#include <fstream>
#include <memory>
#include <algorithm>

#include "extension.h"

#include <bot/basebot.h>
#include <bot/bot_debug_shared.h>

#include <engine/ivdebugoverlay.h>
#include <mods/basemod.h>
#include <extplayer.h>
#include <util/helpers.h>
#include <util/librandom.h>
#include "manager.h"

#ifdef EXT_DEBUG
#include <sdkports/debugoverlay_shared.h>
#include <navmesh/nav.h>
#include <navmesh/nav_area.h>
#include <navmesh/nav_mesh.h>
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

#undef max // valve mathlib conflict fix
#undef min
#undef clamp

constexpr auto BOT_QUOTA_UPDATE_INTERVAL = 2.0f;

ConVar sm_navbot_quota_mode("sm_navbot_quota_mode", "normal", FCVAR_GAMEDLL, "NavBot bot quota mode. \n'normal' = Keep N number of bots in the game.\n'fill' = Fill to N bots, remove to make space for human players");

ConVar sm_navbot_quota_quantity("sm_navbot_quota_quantity", "0", FCVAR_GAMEDLL, "Number of bots to add.");

CExtManager::CExtManager() :
	m_bdm()
{
	m_bots.reserve(128); // 128 should be good for most mods
	m_botnames.reserve(256); // reserve space for 256 bot names, vector size will increase if needed
	m_nextbotname = 0U;
	m_botdebugmode = BOTDEBUG_NONE;
	m_quotamode = BotQuotaMode::QUOTA_FIXED;
	m_quotatarget = 0;
	m_quotaupdatetime = TIME_TO_TICKS(BOT_QUOTA_UPDATE_INTERVAL);
}

CExtManager::~CExtManager()
{
	m_bots.clear();
}

void CExtManager::OnAllLoaded()
{
	AllocateMod();

	LoadBotNames();
	m_bdm.LoadProfiles();

	smutils->LogMessage(myself, "Extension fully loaded. Source Engine '%i'. Detected Mod: '%s'", SOURCE_ENGINE, m_mod->GetModName());
	
	m_wim.LoadConfigFile();
}

void CExtManager::Frame()
{
	if (--m_quotaupdatetime <= 0)
	{
		m_quotaupdatetime = TIME_TO_TICKS(BOT_QUOTA_UPDATE_INTERVAL);

		UpdateBotQuota();
	}

	for (auto& botptr : m_bots)
	{
		auto bot = botptr.get();
		bot->PlayerThink();
	}

	for (auto& player : m_players)
	{
		player.get()->PlayerThink();
	}

	m_mod->Frame();
}

void CExtManager::OnClientPutInServer(int client)
{
	auto edict = gamehelpers->EdictOfIndex(client);

	if (edict == nullptr)
	{
		smutils->LogError(myself, "CExtManager::OnClientPutInServer failed to convert client index to edict pointer!");
		return;
	}

	auto gp = playerhelpers->GetGamePlayer(client);

	if (gp->IsSourceTV() || gp->IsReplay())
		return; // Don't care about sourcetv bots

	if (!IsNavBot(client))
	{
		// Add non navbot clients (humans and other bots) to the player lists so we can update their last known nav areas.
		m_players.emplace_back(std::make_unique<CBaseExtPlayer>(edict));
	}

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


	bool isbot = GetBotByIndex(client) != nullptr;

	if (isbot == true)
	{
		auto botit = m_bots.end();

		for (auto it = m_bots.begin(); it != m_bots.end(); it++)
		{
			auto& botptr = *it;
			auto bot = botptr.get();

			if (bot->GetIndex() == client)
			{
				botit = it;
				break;
			}
		}

		if (botit != m_bots.end())
		{
			m_bots.erase(botit);
		}
	}

	auto playerit = m_players.end();

	for (auto it = m_players.end(); it != m_players.end(); it++)
	{
		auto& i = *it;
		auto player = i.get();

		if (player->GetIndex() == client)
		{
			playerit = it;
			break;
		}
	}

	if (playerit != m_players.end())
	{
		m_players.erase(playerit);
	}
}

void CExtManager::OnMapStart()
{
	TheNavMesh->OnMapStart();
	m_mod->OnMapStart();

	if (m_botnames.size() != 0)
	{
		// get a new index on every map load
		m_nextbotname = librandom::generate_random_uint(0, m_botnames.size() - 1);
	}
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
#else
	m_mod = std::make_unique<CBaseMod>();
#endif // SOURCE_ENGINE == SE_TF2
}

CBaseMod* CExtManager::GetMod()
{
	return m_mod.get();
}

void CExtManager::NotifyRegisterGameEvents()
{
	m_mod->RegisterGameEvents();
}

CBaseBot* CExtManager::GetBotByIndex(int index)
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

CBaseExtPlayer* CExtManager::GetPlayerByIndex(int index)
{
	for (auto& playerptr : m_players)
	{
		auto player = playerptr.get();

		if (player->GetIndex() == index)
		{
			return player;
		}
	}

	return nullptr;
}

bool CExtManager::IsNavBot(const int client) const
{
	for (auto& botptr : m_bots)
	{
		if (botptr.get()->GetIndex() == client)
		{
			return true;
		}
	}

	return false;
}

void CExtManager::AddBot()
{
	const char* name = nullptr;

	if (m_botnames.size() == 0)
	{
		char botname[30]{};
		std::sprintf(botname, "SMNav Bot #%04d", librandom::generate_random_int(0, 9999));
		name = botname;
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

	// Tell the bot manager to create a new bot. OnClientPutInServer is too late to catch the bot being created
	auto edict = botmanager->CreateBot(name);

	if (edict == nullptr)
	{
		smutils->LogError(myself, "Game bot manager failed to create a new bot!");
		return;
	}

	// Create a new bot instance
	auto mod = m_mod.get();
	auto& botptr = m_bots.emplace_back(mod->AllocateBot(edict));
	auto bot = botptr.get();

#ifdef EXT_DEBUG
	// the base bot doesn't allocate these on the constructor
	// to allow debugging these interface, we have to call these functions at least once to create them
	
	bot->GetControlInterface();
	bot->GetMovementInterface();
	bot->GetSensorInterface();
	bot->GetBehaviorInterface();
#endif // EXT_DEBUG

	rootconsole->ConsolePrint("Bot added to the game.");
}

void CExtManager::RemoveRandomBot(const char* message)
{
	auto& botptr = m_bots[randomgen->GetRandomInt<size_t>(0U, m_bots.size() - 1)];
	auto player = playerhelpers->GetGamePlayer(botptr->GetIndex());
	player->Kick(message);
}

void CExtManager::RemoveAllBots(const char* message)
{
	for (auto& botptr : m_bots)
	{
		auto player = playerhelpers->GetGamePlayer(botptr->GetIndex());
		player->Kick(message);
	}
}

void CExtManager::LoadBotNames()
{
	char path[PLATFORM_MAX_PATH]{};

	smutils->BuildPath(SourceMod::PathType::Path_SM, path, sizeof(path), "configs/navbot/bot_names.cfg");

	if (std::filesystem::exists(path) == false)
	{
		smutils->LogError(myself, "Could not load bot name list, file \"%s\" doesn't exists!", path);
		return;
	}
	
	m_botnames.clear(); // clear vector if we're reloading the name list

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
		line.erase(std::remove(line.begin(), line.end(), '\r'), line.cend());
		line.erase(std::remove(line.begin(), line.end(), '\n'), line.cend());

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

#ifdef EXT_DEBUG
	for (auto& name : m_botnames)
	{
		auto szname = name.c_str();
		rootconsole->ConsolePrint("Bot name: %s", szname);
	}
#endif // EXT_DEBUG

	rootconsole->ConsolePrint("[SMNav] Bot name list loaded with %i names.", m_botnames.size());
}

void CExtManager::UpdateBotQuota()
{
	auto mode = sm_navbot_quota_mode.GetString();

	if (strncasecmp(mode, "normal", 6) == 0)
	{
		m_quotamode = BotQuotaMode::QUOTA_FIXED;
	}
	else if (strncasecmp(mode, "fill", 4) == 0)
	{
		m_quotamode = BotQuotaMode::QUOTA_FILL;
	}
	else
	{
		m_quotamode = BotQuotaMode::QUOTA_FIXED;
		smutils->LogError(myself, "Unknown bot quota mode \"%s\"!", mode);
	}

	m_quotatarget = sm_navbot_quota_quantity.GetInt();

	m_quotatarget = std::clamp(m_quotatarget, 0, gpGlobals->maxClients - 1); // limit max bots to server maxplayers - 1

	if (m_quotatarget == 0)
		return;

	int humans = 0;
	int navbots = 0;
	int otherbots = 0;

	UtilHelpers::ForEachPlayer([this, &humans, &navbots, &otherbots](int client, edict_t* entity, SourceMod::IGamePlayer* player) -> void {
		if (player->IsInGame())
		{
			if (player->IsFakeClient())
			{
				if (IsNavBot(client))
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
	});

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

CON_COMMAND(sm_navbot_reload_name_list, "Reloads the bot name list")
{
	extern CExtManager* extmanager;
	extmanager->LoadBotNames();
}

CON_COMMAND_F(sm_navbot_debug, "Toggles between debug modes", FCVAR_CHEAT)
{
	if (args.ArgC() <= 1)
	{
		rootconsole->ConsolePrint("Available debug options: STOPALL, SENSOR, TASKS, LOOK, PATH, EVENTS, MOVEMENT, ERRORS");
		rootconsole->ConsolePrint("Usage: sm_navbot_debug <OPTION1> <OPTION2> ...");
		return;
	}

	extern CExtManager* extmanager;
	
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
		else
		{
			rootconsole->ConsolePrint("Unknown option \"%s\".", option);
			rootconsole->ConsolePrint("Available debug options: STOPALL, SENSOR, TASKS, LOOK, PATH, EVENTS, MOVEMENT, ERRORS");
			rootconsole->ConsolePrint("Usage: sm_navbot_debug <OPTION1> <OPTION2> ...");
		}
	}
}

