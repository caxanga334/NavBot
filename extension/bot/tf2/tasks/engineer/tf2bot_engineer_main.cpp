#include NAVBOT_PCH_FILE
#include <array>
#include <string_view>
#include <limits>
#include <extension.h>
#include <util/helpers.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tasks_shared/bot_shared_attack_enemy.h>
#include <bot/tasks_shared/bot_shared_retreat_from_threat.h>
#include "tf2bot_engineer_sentry_combat.h"
#include "tf2bot_engineer_repair_object.h"
#include "tf2bot_engineer_upgrade_object.h"
#include "tf2bot_engineer_nest.h"
#include "tf2bot_engineer_main.h"

#undef clamp
#undef max
#undef min

CTF2BotEngineerMainTask::CTF2BotEngineerMainTask()
{
}

AITask<CTF2Bot>* CTF2BotEngineerMainTask::InitialNextTask(CTF2Bot* bot)
{
	return new CTF2BotEngineerNestTask;
}

TaskResult<CTF2Bot> CTF2BotEngineerMainTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_updateBuildingsTimer.IsElapsed())
	{
		m_updateBuildingsTimer.Start(1.0f);
		bot->FindMyBuildings();
	}

	if (bot->IsCarryingObject())
	{
		return Continue();
	}

	CBaseEntity* mysentry;
	CBaseEntity* mydispenser;
	CBaseEntity* myentrance;
	CBaseEntity* myexit;

	bot->GetMyBuildings(&mysentry, &mydispenser, &myentrance, &myexit);

	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ONLY_VISIBLE_THREATS);

	if (threat)
	{
		if (mysentry)
		{
			return PauseFor(new CTF2BotEngineerSentryCombatTask(), "Combat with visible enemy!");
		}
		else
		{
			if (CBaseBot::s_botrng.GetRandomChance(bot->GetDifficultyProfile()->GetAggressiveness()))
			{
				return PauseFor(new CBotSharedAttackEnemyTask<CTF2Bot, CTF2BotPathCost>(bot), "Attacking visible threat!");
			}
			else
			{
				return PauseFor(new CBotSharedRetreatFromThreatTask<CTF2Bot, CTF2BotPathCost>(bot, botsharedutils::SelectRetreatArea::RetreatAreaPreference::FURTHEST, 3.0f, 90.0f, 512.0f, 4096.0f), "Retreating from visible threat!");
			}
		}
	}

	if (m_repairCheckTimer.IsElapsed())
	{
		if (mysentry && tf2lib::BuildingNeedsToBeRepaired(mysentry))
		{
			return PauseFor(new CTF2BotEngineerRepairObjectTask(mysentry), "Repairing my sentry gun!");
		}

		if (mydispenser && tf2lib::BuildingNeedsToBeRepaired(mydispenser))
		{
			return PauseFor(new CTF2BotEngineerRepairObjectTask(mydispenser), "Repairing my dispenser!");
		}

		if (myexit && tf2lib::BuildingNeedsToBeRepaired(myexit)) // assume the exit is closer
		{
			return PauseFor(new CTF2BotEngineerRepairObjectTask(myexit), "Repairing my teleporter exit!");
		}

		if (myentrance && tf2lib::BuildingNeedsToBeRepaired(myentrance))
		{
			return PauseFor(new CTF2BotEngineerRepairObjectTask(myentrance), "Repairing my teleporter entrance!");
		}
	}

	return Continue();
}
