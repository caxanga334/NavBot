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
	gamemod = new CBaseMod;
}

CBaseMod* CExtManager::GetMod()
{
	return gamemod;
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
#endif
