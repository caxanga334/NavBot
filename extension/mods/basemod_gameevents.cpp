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