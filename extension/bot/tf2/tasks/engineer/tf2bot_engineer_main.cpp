#include NAVBOT_PCH_FILE
#include <array>
#include <string_view>
#include <limits>
#include <extension.h>
#include <util/helpers.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
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

	if (m_repairCheckTimer.IsElapsed())
	{
		m_repairCheckTimer.Start(0.250f);

		CBaseEntity* mysentry;
		CBaseEntity* mydispenser;
		CBaseEntity* myentrance;
		CBaseEntity* myexit;

		bot->GetMyBuildings(&mysentry, &mydispenser, &myentrance, &myexit);

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
