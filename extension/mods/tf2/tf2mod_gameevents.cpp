#include <extension.h>
#include <bot/tf2/tf2bot.h>
#include <manager.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include "teamfortress2mod.h"
#include "tf2mod_gameevents.h"

/*
void CTF2PlayerDeathEvent::OnGameEvent(IGameEvent* gameevent)
{
	int victim = playerhelpers->GetClientOfUserId(gameevent->GetInt("userid"));
	int killer = playerhelpers->GetClientOfUserId(gameevent->GetInt("attacker"));
	int deathflags = gameevent->GetInt("death_flags");
	bool silentkill = gameevent->GetBool("silent_kill");
	bool fakedeath = false;
	int victim_team = 0;

	if (deathflags & TeamFortress2::TF_DEATHFLAG_DEADRINGER)
	{
		fakedeath = true;
	}

	edict_t* pVictim = nullptr;
	edict_t* pKiller = nullptr;

	if (victim > 0)
	{
		pVictim = gamehelpers->EdictOfIndex(victim);
	}

	if (killer > 0)
	{
		pKiller = gamehelpers->EdictOfIndex(killer);
	}

	if (!pVictim)
		return; // failed to retreive victim

	if (fakedeath)
	{
		entprops->GetEntProp(victim, Prop_Send, "m_iTeamNum", victim_team);
	}

	auto shouldfire = [&fakedeath, &victim_team](CBaseBot* bot) -> bool {
		if (victim_team <= TEAM_SPECTATOR)
			return true; // always notify if team lookup failed or invalid team

		if (fakedeath && victim_team != bot->GetCurrentTeamIndex())
			return false; // don't notify bots of different teams on fake deaths

		return true;
	};

	auto& thebots = extmanager->GetAllBots();

	for (auto& botptr : thebots)
	{
		auto bot = botptr.get();

		if (!shouldfire(bot))
			continue;

		if (bot->GetIndex() == victim)
		{
			bot->Killed(); // notify bot
			bot->OnKilled(pKiller); // Send event for Tasks
		}
		else
		{
			bot->OnOtherKilled(pVictim, pKiller); // Notify other bots
		}
	}
}
*/