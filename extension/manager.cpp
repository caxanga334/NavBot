#include "extension.h"

#ifdef SMNAV_FEAT_BOT
#include <bot/basebot.h>
#endif // SMNAV_FEAT_BOT

#include <engine/ivdebugoverlay.h>
#include <mods/basemod.h>
#include <extplayer.h>
#include "manager.h"

CBaseMod* gamemod = nullptr;

CExtManager::CExtManager()
{
	for (int i = 0; i < SMNAV_MAX_PLAYERS; i++)
	{
		m_players[i] = nullptr;
	}
}

CExtManager::~CExtManager()
{
	for (int i = 0; i < SMNAV_MAX_PLAYERS; i++)
	{
		if (m_players[i])
		{
			delete m_players[i];
		}

		m_players[i] = nullptr;
	}

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
	for (int i = 0; i < SMNAV_MAX_PLAYERS; i++)
	{
		if (m_players[i])
		{
			m_players[i]->PlayerThink(); // Run player think
		}
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

	m_players[client] = gamemod->AllocatePlayer(edict);
}

void CExtManager::OnClientDisconnect(int client)
{
	if (m_players[client])
	{
		delete m_players[client];
		m_players[client] = nullptr;
	}
}

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
	return m_players[index]->MyBotPointer();
}

// For debug stuff, NDEBUG is used since it's not possible to use DEBUG (compiler errors from the SDK)
#ifndef NDEBUG
CON_COMMAND(smnav_debug_vectors, "[LISTEN SERVER] Debug player vectors")
{
	extern CExtManager* extmanager;
	extern IVDebugOverlay* debugoverlay;

	auto player = extmanager->GetPlayerByIndex(1);

	static Vector mins(-4.0f, -4.0f, -4.0f);
	static Vector maxs(4.0f, 4.0f, 4.0f);

	auto origin = player->GetAbsOrigin();
	auto angles = player->GetAbsAngles();
	auto eyepos = player->GetEyeOrigin();
	auto eyeangles = player->GetEyeAngles();

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
#endif // !NDEBUG
