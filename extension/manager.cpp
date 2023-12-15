#include <vector>
#include <stdexcept>
#include <filesystem>
#include <string>
#include <fstream>
#include <memory>

#include "extension.h"

#include <bot/basebot.h>

#include <engine/ivdebugoverlay.h>
#include <mods/basemod.h>
#include <extplayer.h>
#include <util/librandom.h>
#include "manager.h"

#ifdef EXT_DEBUG
#include <sdkports/debugoverlay_shared.h>
#include <navmesh/nav.h>
#include <navmesh/nav_area.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_pathfind.h>
#include <bot/interfaces/path/basepath.h>
#endif // EXT_DEBUG

#if SOURCE_ENGINE == SE_TF2
#include <mods/tf2/teamfortress2mod.h>
#endif // SOURCE_ENGINE == SE_TF2

#if SOURCE_ENGINE == SE_DODS
#include <mods/dods/dayofdefeatsourcemod.h>
#endif // SOURCE_ENGINE == SE_DODS

extern IBotManager* botmanager;


CExtManager::CExtManager()
{
	m_bots.reserve(128); // 128 should be good for most mods
	m_botnames.reserve(256); // reserve space for 256 bot names, vector size will increase if needed
	m_nextbotname = 0U;
	m_botdebugmode = BOTDEBUG_NONE;
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

	smutils->LogMessage(myself, "Extension fully loaded. Source Engine '%i'. Detected Mod: '%s'", SOURCE_ENGINE, m_mod.get()->GetModName());
}

void CExtManager::Frame()
{
	for (auto& botptr : m_bots)
	{
		auto bot = botptr.get();
		bot->PlayerThink();
	}

	m_mod.get()->Frame();
}

void CExtManager::OnClientPutInServer(int client)
{
	auto edict = gamehelpers->EdictOfIndex(client);

	if (edict == nullptr)
	{
		smutils->LogError(myself, "CExtManager::OnClientPutInServer failed to convert client index to edict pointer!");
		return;
	}

#ifdef EXT_DEBUG
	auto gp = playerhelpers->GetGamePlayer(client);
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
}

void CExtManager::OnMapStart()
{
	m_mod.get()->OnMapStart();

	if (m_botnames.size() != 0)
	{
		// get a new index on every map load
		m_nextbotname = librandom::generate_random_uint(0, m_botnames.size() - 1);
	}
}

void CExtManager::OnMapEnd()
{
	m_mod.get()->OnMapEnd();
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
	m_mod.get()->RegisterGameEvents();
}

CBaseBot* CExtManager::GetBotByIndex(int index)
{
	for (auto& botptr : m_bots)
	{
		auto bot = botptr.get();
		if (bot->GetIndex() == index)
		{
			return botptr.get();
		}
	}

	return nullptr;
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

		if (std::isspace(line[0]))
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

CON_COMMAND(sm_navbot_reload_name_list, "Reloads the bot name list")
{
	extern CExtManager* extmanager;
	extmanager->LoadBotNames();
}

CON_COMMAND_F(sm_navbot_debug, "Toggles between debug modes", FCVAR_CHEAT)
{
	if (args.ArgC() < 1)
	{
		rootconsole->ConsolePrint("Available debug options: STOPALL, SENSOR, TASKS, LOOK, PATH, EVENTS, LOCOMOTION, ERRORS");
		rootconsole->ConsolePrint("Usage: sm_navbot_debug <OPTION1> <OPTION2> ...");
		return;
	}

	extern CExtManager* extmanager;
	
	for (int i = 1; i > args.ArgC(); i++)
	{
		auto option = args.Arg(i);

		if (strncasecmp(option, "STOPALL", 6) == 0)
		{
			extmanager->StopAllDebugging();
			rootconsole->ConsolePrint("Stopped debugging");
			return;
		}
		else if (strncasecmp(option, "SENSOR", 6) == 0)
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
		else if (strncasecmp(option, "LOCOMOTION", 10) == 0)
		{
			extmanager->ToggleDebugOption(BOTDEBUG_LOCOMOTION);
			rootconsole->ConsolePrint("Toggle Debugging Bot Locomotion");
		}
		else if (strncasecmp(option, "ERRORS", 6) == 0)
		{
			extmanager->ToggleDebugOption(BOTDEBUG_ERRORS);
			rootconsole->ConsolePrint("Toggle Debugging Bot Error");
		}
		else
		{
			rootconsole->ConsolePrint("Unknown option \"%s\".", option);
			rootconsole->ConsolePrint("Available debug options: STOPALL, SENSOR, TASKS, LOOK, PATH, EVENTS, LOCOMOTION, ERRORS");
			rootconsole->ConsolePrint("Usage: sm_navbot_debug <OPTION1> <OPTION2> ...");
		}
	}
}

#ifdef EXT_DEBUG
CON_COMMAND(sm_navbot_debug_vectors, "[LISTEN SERVER] Debug player vectors")
{
	extern CExtManager* extmanager;
	extern IVDebugOverlay* debugoverlay;

	auto edict = gamehelpers->EdictOfIndex(1);

	if (edict == nullptr)
	{
		return;
	}

	// CBaseExtPlayer can be used for both players and bots
	CBaseExtPlayer player(edict);

	static Vector mins(-4.0f, -4.0f, -4.0f);
	static Vector maxs(4.0f, 4.0f, 4.0f);

	auto origin = player.GetAbsOrigin();
	auto angles = player.GetAbsAngles();
	auto eyepos = player.GetEyeOrigin();
	auto eyeangles = player.GetEyeAngles();

	rootconsole->ConsolePrint("Origin: <%3.2f> <%3.2f> <%3.2f>", origin.x, origin.y, origin.z);
	rootconsole->ConsolePrint("Angles: <%3.2f> <%3.2f> <%3.2f>", angles.x, angles.y, angles.z);
	rootconsole->ConsolePrint("Eye Position: <%3.2f> <%3.2f> <%3.2f>", eyepos.x, eyepos.y, eyepos.z);
	rootconsole->ConsolePrint("Eye Angles: <%3.2f> <%3.2f> <%3.2f>", eyeangles.x, eyeangles.y, eyeangles.z);

	debugoverlay->AddBoxOverlay(origin, mins, maxs, angles, 0, 255, 0, 200, 15.0f);
	debugoverlay->AddBoxOverlay(eyepos, mins, maxs, eyeangles, 0, 0, 255, 200, 15.0f);

	static Vector forward;

	AngleVectors(eyeangles, &forward);

	forward = forward * 300.0f;

	debugoverlay->AddLineOverlay(eyepos, eyepos + forward, 255, 0, 0, true, 15.0f);
}

CON_COMMAND_F(sm_navbot_debug_do_not_use, "Do not even think about executing this command.", FCVAR_CHEAT)
{
	throw std::runtime_error("Exception test!");
}

CON_COMMAND(sm_navbot_debug_event_propagation, "Event propagation test")
{
	extern CExtManager* extmanager;
	auto& bots = extmanager->GetAllBots();

	for (auto& botptr : bots)
	{
		auto bot = botptr.get();
		bot->OnTestEventPropagation();
		bot->GetBehaviorInterface()->ShouldFreeRoam(bot);
	}
}

CON_COMMAND(sm_navbot_debug_pathfind, "Path finding debug.")
{
	static Vector start;
	static Vector end;
	static bool reset = false;

	auto edict = gamehelpers->EdictOfIndex(1);

	if (edict == nullptr)
	{
		return;
	}

	// CBaseExtPlayer can be used for both players and bots
	CBaseExtPlayer player(edict);

	if (reset == false)
	{
		start = player.GetAbsOrigin();
		reset = true;
		return;
	}

	reset = false;
	end = player.GetAbsOrigin();

	CNavArea* startArea = TheNavMesh->GetNearestNavArea(start, 256.0f, true, true);

	if (startArea == nullptr)
	{
		rootconsole->ConsolePrint("Path find: Failed to get starting area!");
		return;
	}

	CNavArea* goalArea = TheNavMesh->GetNearestNavArea(end, 256.0f, true, true);

	ShortestPathCost cost;
	CNavArea* closest = nullptr;
	bool path = NavAreaBuildPath(startArea, goalArea, &end, cost, &closest, 10000.0f, -2);

	CNavArea* from = nullptr;
	CNavArea* to = nullptr;

	for (CNavArea* area = closest; area != nullptr; area = area->GetParent())
	{
		if (from == nullptr) // set starting area;
		{
			from = area;
			continue;
		}

		to = area;

		NDebugOverlay::HorzArrow(from->GetCenter(), to->GetCenter(), 4.0f, 0, 255, 0, 255, true, 20.0f);
		
		from = to;
	}
}

CON_COMMAND(sm_navbot_debug_botpath, "Debug bot path")
{
	CBaseBot* bot = nullptr;
	extern CGlobalVars* gpGlobals;
	extern CExtManager* extmanager;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		bot = extmanager->GetBotByIndex(i);

		if (bot != nullptr)
		{
			break;
		}
	}

	if (bot == nullptr)
	{
		rootconsole->ConsolePrint("Add a bot to the game first!");
		return;
	}

	auto edict = gamehelpers->EdictOfIndex(1);

	if (edict == nullptr)
	{
		return;
	}

	// CBaseExtPlayer can be used for both players and bots
	CBaseExtPlayer player(edict);
	ShortestPathCost cost;
	CPath path;
	Vector start = player.GetAbsOrigin();

	path.ComputePathToPosition(bot, start, cost, 0.0f, true);
	path.DrawFullPath(30.0f);

	rootconsole->ConsolePrint("Showing path from bot position to your position.");
}

CON_COMMAND(sm_navbot_debug_rng, "Debug random number generator")
{
	for (int i = 0; i < 15; i++)
	{
		int n = librandom::generate_random_int(0, 100);
		rootconsole->ConsolePrint("Random Number: %i", n);
	}
}
#endif // EXT_DEBUG
