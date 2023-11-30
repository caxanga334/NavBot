#include <vector>
#include <stdexcept>

#include "extension.h"

#ifdef SMNAV_FEAT_BOT
#include <bot/basebot.h>
#endif // SMNAV_FEAT_BOT

#include <engine/ivdebugoverlay.h>
#include <mods/basemod.h>
#include <extplayer.h>
#include "manager.h"

#ifdef SMNAV_DEBUG
#include <sdkports/debugoverlay_shared.h>
#include <navmesh/nav.h>
#include <navmesh/nav_area.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_pathfind.h>
#include <bot/interfaces/path/basepath.h>
#include <util/librandom.h>
#endif // SMNAV_DEBUG

#if SOURCE_ENGINE == SE_TF2
#include <mods/tf2/teamfortress2mod.h>
#endif // SOURCE_ENGINE == SE_TF2

#if SOURCE_ENGINE == SE_DODS
#include <mods/dods/dayofdefeatsourcemod.h>
#endif // SOURCE_ENGINE == SE_DODS


CBaseMod* gamemod = nullptr;

#ifdef SMNAV_FEAT_BOT
extern IBotManager* botmanager;
#endif // SMNAV_FEAT_BOT

CExtManager::CExtManager()
{
	m_bots.reserve(128); // 128 should be good for most mods
}

CExtManager::~CExtManager()
{
	for (auto bot : m_bots)
	{
		delete bot;
	}

	m_bots.clear();

	if (gamemod)
	{
		delete gamemod;
		gamemod = nullptr;
	}
}

void CExtManager::OnAllLoaded()
{
	AllocateMod();

	smutils->LogMessage(myself, "Extension fully loaded. Source Engine '%i'. Detected Mod: '%s'", SOURCE_ENGINE, gamemod->GetModName());
}

void CExtManager::Frame()
{
	for (auto bot : m_bots)
	{
		bot->PlayerThink();
	}

	gamemod->Frame();
}

void CExtManager::OnClientPutInServer(int client)
{
	auto edict = gamehelpers->EdictOfIndex(client);

	if (edict == nullptr)
	{
		smutils->LogError(myself, "CExtManager::OnClientPutInServer failed to convert client index to edict pointer!");
		return;
	}

#ifdef SMNAV_DEBUG
	auto gp = playerhelpers->GetGamePlayer(client);
	auto auth = gp->GetAuthString(true);

	if (auth == nullptr)
	{
		auth = "NULL";
	}

	smutils->LogMessage(myself, "OnClientPutInServer -- %i %p '%s'", client, edict, auth);
#endif // SMNAV_DEBUG
}

void CExtManager::OnClientDisconnect(int client)
{
#ifdef SMNAV_FEAT_BOT

	auto bot = GetBotByIndex(client);

	if (bot != nullptr)
	{
		auto start = m_bots.begin();
		auto end = m_bots.end();
		auto it = std::find(start, end, bot);

#ifdef SMNAV_DEBUG
		if (it == end)
		{
			// something went very wrong if the code enters here
			// throw and crash
			throw std::runtime_error("Got valid CBaseBot pointer but end iterator on CExtManager::OnClientDisconnect!");
		}
#endif // SMNAV_DEBUG

		delete bot; // Deallocate the bot
		m_bots.erase(it); // Remove the bot from the vector
	}

#endif // SMNAV_FEAT_BOT
}

// Detect current mod and initializes it
void CExtManager::AllocateMod()
{
	// Don't check game folder name unless the SDK supports multiple mods (SDK2013, EP2 (Orange box), EP1 (Original))

#if SOURCE_ENGINE == SE_TF2
	gamemod = new CTeamFortress2Mod;
#elif SOURCE_ENGINE == SE_DODS
	gamemod = new CDayOfDefeatSourceMod;
#else
	gamemod = new CBaseMod;
#endif // SOURCE_ENGINE == SE_TF2
}

CBaseMod* CExtManager::GetMod()
{
	return gamemod;
}

void CExtManager::NotifyRegisterGameEvents()
{
	gamemod->RegisterGameEvents();
}

CBaseBot* CExtManager::GetBotByIndex(int index)
{
	for (auto bot : m_bots)
	{
		if (bot->GetIndex() == index)
		{
			return bot;
		}
	}

	return nullptr;
}

void CExtManager::AddBot()
{
#ifdef SMNAV_FEAT_BOT
	// Tell the bot manager to create a new bot. OnClientPutInServer is too late to catch the bot being created
	auto edict = botmanager->CreateBot("SMBot++");

	if (edict == nullptr)
	{
		smutils->LogError(myself, "Game bot manager failed to create a new bot!");
		return;
	}

	// Create a new bot instance
	CBaseBot* bot = gamemod->AllocateBot(edict);
	m_bots.push_back(bot); // Add it to the bot list

#ifdef SMNAV_DEBUG
	// the base bot doesn't allocate these on the constructor
	// to allow debugging these interface, we have to call these functions at least once to create them
	bot->GetControlInterface();
	bot->GetMovementInterface();
	bot->GetSensorInterface();
	bot->GetBehaviorInterface();
#endif // SMNAV_DEBUG


	rootconsole->ConsolePrint("Bot added to the game.");
#endif // SMNAV_FEAT_BOT
}

#ifdef SMNAV_DEBUG
CON_COMMAND(smnav_debug_vectors, "[LISTEN SERVER] Debug player vectors")
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

CON_COMMAND_F(smnav_debug_do_not_use, "Do not even think about executing this command.", FCVAR_CHEAT)
{
	throw std::runtime_error("Exception test!");
}

CON_COMMAND(smnav_debug_event_propagation, "Event propagation test")
{
	extern CExtManager* extmanager;
	auto& bots = extmanager->GetAllBots();

	for (auto bot : bots)
	{
		bot->OnTestEventPropagation();
	}

	for (auto bot : bots)
	{
		bot->GetBehaviorInterface()->ShouldFreeRoam(bot);
	}
}

CON_COMMAND(smnav_debug_pathfind, "Path finding debug.")
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

CON_COMMAND(smnav_debug_botpath, "Debug bot path")
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

CON_COMMAND(smnav_debug_rng, "Debug random number generator")
{
	for (int i = 0; i < 15; i++)
	{
		int n = random::generate_random_int(0, 100);
		rootconsole->ConsolePrint("Random Number: %i", n);
	}
}
#endif // SMNAV_DEBUG
