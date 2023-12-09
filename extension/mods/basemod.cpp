#include <extension.h>
#include <extplayer.h>
#include <bot/basebot.h>
#include <core/eventmanager.h>
#include "basemod_gameevents.h"
#include "basemod.h"

CBaseMod::CBaseMod()
{
}

CBaseMod::~CBaseMod()
{
}

CBaseBot* CBaseMod::AllocateBot(edict_t* edict)
{
	return new CBaseBot(edict);
}

void CBaseMod::RegisterGameEvents()
{
	auto evmanager = GetGameEventManager();

	evmanager->RegisterEventReceiver(new CPlayerSpawnEvent);
}

const Vector& CBaseMod::GetPlayerHullMins()
{
	static Vector mins(-16.0f, -16.0f, 0.0f);
	return mins;
}

const Vector& CBaseMod::GetPlayerHullMaxs()
{
	static Vector maxs(16.0f, 16.0f, 72.0f);
	return maxs;
}
