#include <extension.h>
#include <bot/basebot.h>
#include <manager.h>
#include "basemod_gameevents.h"

extern CExtManager* extmanager;

CPlayerSpawnEvent::CPlayerSpawnEvent() : IEventReceiver("player_spawn", Mods::MOD_ALL)
{
}

void CPlayerSpawnEvent::OnGameEvent(IGameEvent* gameevent)
{
	// set default value to a number that will probably never be sent, so we can catch if the userid key doesn't exists
	constexpr int DEFAULT_VALUE = -25842;

	int userid = gameevent->GetInt("userid", DEFAULT_VALUE);

	if (userid == DEFAULT_VALUE)
	{
		smutils->LogError(myself, "Received player_spawn but event doesn't have 'userid' key!");
		return;
	}

	int client = playerhelpers->GetClientOfUserId(userid);

	if (client > 0)
	{
		CBaseBot* bot = extmanager->GetBotByIndex(client);

		if (bot != nullptr)
		{
			bot->Spawn();
		}
	}
}

void CPlayerHurtEvent::OnGameEvent(IGameEvent* gameevent)
{
	int victim = playerhelpers->GetClientOfUserId(gameevent->GetInt("userid"));
	int attacker = playerhelpers->GetClientOfUserId(gameevent->GetInt("attacker"));

	edict_t* pVictim = nullptr;
	edict_t* pAttacker = nullptr;

	if (victim > 0)
	{
		pVictim = gamehelpers->EdictOfIndex(victim);
	}

	if (attacker > 0)
	{
		pAttacker = gamehelpers->EdictOfIndex(attacker);
	}

	if (!pVictim)
		return;

	auto bot = extmanager->GetBotByIndex(victim);
	bot->OnInjured(pAttacker);
}
