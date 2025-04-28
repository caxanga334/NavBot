#include <limits>
#include <cstring>

#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <mods/tf2/teamfortress2mod.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/bot_shared_convars.h>
#include "tf2bot_tactical.h"
#include "tf2bot_find_ammo_task.h"
#include "tf2bot_find_health_task.h"
#include "tf2bot_use_teleporter.h"
#include <bot/tasks_shared/bot_shared_escort_entity.h>
#include "tf2bot_scenario_task.h"

#undef max
#undef min
#undef clamp // undef mathlib macros

AITask<CTF2Bot>* CTF2BotTacticalTask::InitialNextTask(CTF2Bot* bot)
{
	return new CTF2BotScenarioTask;
}

TaskResult<CTF2Bot> CTF2BotTacticalTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_ammochecktimer.Start(5.0f);
	m_healthchecktimer.Start(5.0f);
	m_teleportertimer.Start(3.0f);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotTacticalTask::OnTaskUpdate(CTF2Bot* bot)
{
	// low ammo and health check
	if (bot->GetBehaviorInterface()->ShouldRetreat(bot) != ANSWER_NO && bot->GetBehaviorInterface()->ShouldHurry(bot) != ANSWER_YES)
	{
		const float healthratio = sm_navbot_bot_low_health_ratio.GetFloat();

		if (healthratio >= 0.1f && m_healthchecktimer.IsElapsed())
		{
			m_healthchecktimer.StartRandom(1.0f, 5.0f);

			if (bot->GetHealthPercentage() <= healthratio)
			{
				return PauseFor(new CTF2BotFindHealthTask, "I'm low on health!");
			}
		}

		if (!sm_navbot_bot_no_ammo_check.GetBool() && m_ammochecktimer.IsElapsed())
		{
			m_ammochecktimer.StartRandom(1.0f, 5.0f);

			if (bot->IsAmmoLow())
			{
				return PauseFor(new CTF2BotFindAmmoTask, "I'm low on ammo!");
			}
		}
	}

	if (m_teleportertimer.IsElapsed())
	{
		CBaseEntity* teleporter = nullptr;

		if (CTF2BotUseTeleporterTask::IsPossible(bot, &teleporter))
		{
			m_teleportertimer.StartRandom(75.0f, 135.0f); // don't search again for a while
			return PauseFor(new CTF2BotUseTeleporterTask(teleporter), "Using teleporter!");
		}
		else
		{
			m_teleportertimer.StartRandom(1.0f, 6.0f);
		}
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotTacticalTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_ammochecktimer.Start(5.0f);
	m_healthchecktimer.Start(0.5f);

	return Continue();
}

QueryAnswerType CTF2BotTacticalTask::ShouldRetreat(CBaseBot* base)
{
	CTF2Bot* me = static_cast<CTF2Bot*>(base);

	if (tf2lib::IsPlayerInvulnerable(me->GetIndex()))
	{
		return ANSWER_NO;
	}

	const float healthratio = sm_navbot_bot_low_health_ratio.GetFloat();

	if (healthratio >= 0.1f && me->GetHealthPercentage() <= healthratio)
	{
		return ANSWER_YES;
	}

	if (me->IsAmmoLow())
	{
		return ANSWER_YES;
	}

	return ANSWER_UNDEFINED;
}

TaskEventResponseResult<CTF2Bot> CTF2BotTacticalTask::OnInjured(CTF2Bot* bot, const CTakeDamageInfo& info)
{
	if (bot->GetDifficultyProfile()->GetGameAwareness() >= 15)
	{
		CBaseEntity* inflictor = info.GetInflictor();

		if (inflictor != nullptr)
		{
			int index = gamehelpers->EntityToBCompatRef(inflictor);

			if (index > 0 && index <= MAX_EDICTS)
			{
				Vector pos = UtilHelpers::getWorldSpaceCenter(inflictor);
				bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_DANGER, 2.0f, "Aiming at damage source!");
			}
		}
	}

	return TryContinue(PRIORITY_DONT_CARE);
}

TaskEventResponseResult<CTF2Bot> CTF2BotTacticalTask::OnVoiceCommand(CTF2Bot* bot, CBaseEntity* subject, int command)
{
	int skill = bot->GetDifficultyProfile()->GetGameAwareness();

	if (skill <= 5)
	{
		return TryContinue(PRIORITY_DONT_CARE); // this one doesn't know about VCs yet
	}

	TeamFortress2::TFTeam theirteam = static_cast<TeamFortress2::TFTeam>(entityprops::GetEntityTeamNum(subject));
	TeamFortress2::VoiceCommandsID vcmd = static_cast<TeamFortress2::VoiceCommandsID>(command);

	if (skill >= 75 && theirteam != bot->GetMyTFTeam())
	{
		if (bot->GetRangeTo(subject) < bot->GetSensorInterface()->GetMaxHearingRange())
		{
			CKnownEntity* known = bot->GetSensorInterface()->AddKnownEntity(subject);
			Vector pos = UtilHelpers::getWorldSpaceCenter(subject);
			bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_ALERT, 2.0f, "Looking at hostile voice command source position!");

			if (known)
			{
				// this will update the entity position and mark it as heard
				known->UpdateHeard();
			}
		}
	}

	return TryContinue(PRIORITY_DONT_CARE);
}

