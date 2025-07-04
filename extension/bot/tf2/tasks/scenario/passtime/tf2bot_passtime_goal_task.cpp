#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <sdkports/debugoverlay_shared.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/tf2lib.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "tf2bot_passtime_goal_task.h"

TaskResult<CTF2Bot> CTF2BotPassTimeGoalTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* tower = tf2lib::passtime::GetGoalEntity(TeamFortress2::PassTimeGoalType::TYPE_TOWER, bot->GetMyTFTeam());

	// tower gives more points, priorize it, random chance to ignore it
	if (tower && randomgen->GetRandomInt<int>(0, 4) > 0)
	{
		m_goalent = tower;
		m_goaltype = TeamFortress2::PassTimeGoalType::TYPE_TOWER;

		if (bot->IsDebugging(BOTDEBUG_TASKS))
		{
			META_CONPRINTF("%s CTF2BotPassTimeGoalTask::OnTaskStart GOAL IS TOWER! \n", bot->GetDebugIdentifier());
		}

		return Continue();
	}

	CBaseEntity* hoop = tf2lib::passtime::GetGoalEntity(TeamFortress2::PassTimeGoalType::TYPE_HOOP, bot->GetMyTFTeam());
	CBaseEntity* endzone = tf2lib::passtime::GetGoalEntity(TeamFortress2::PassTimeGoalType::TYPE_ENDZONE, bot->GetMyTFTeam());
	CBaseEntity* goal = nullptr;

	if (hoop && endzone)
	{
		if (randomgen->GetRandomInt<int>(0, 1) != 0)
		{
			goal = hoop;
			m_goaltype = TeamFortress2::PassTimeGoalType::TYPE_HOOP;
		}
		else
		{
			goal = endzone;
			m_goaltype = TeamFortress2::PassTimeGoalType::TYPE_ENDZONE;
		}
	}
	else if (hoop)
	{
		goal = hoop;
		m_goaltype = TeamFortress2::PassTimeGoalType::TYPE_HOOP;
	}
	else
	{
		goal = endzone;
		m_goaltype = TeamFortress2::PassTimeGoalType::TYPE_ENDZONE;
	}

	if (!goal)
	{
		return Done("Failed to find goal entity!");
	}

	if (bot->IsDebugging(BOTDEBUG_TASKS))
	{
		if (m_goaltype == TeamFortress2::PassTimeGoalType::TYPE_HOOP)
		{
			META_CONPRINTF("%s CTF2BotPassTimeGoalTask::OnTaskStart GOAL IS HOOP! \n", bot->GetDebugIdentifier());
		}
		else if (m_goaltype == TeamFortress2::PassTimeGoalType::TYPE_ENDZONE)
		{
			META_CONPRINTF("%s CTF2BotPassTimeGoalTask::OnTaskStart GOAL IS ENDZONE! \n", bot->GetDebugIdentifier());
		}
	}

	m_goalent = goal;

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotPassTimeGoalTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* goal = m_goalent.Get();

	if (!goal)
	{
		return Done("Goal ent is NULL!");
	}

	if (!bot->IsCarryingThePassTimeJack())
	{
		return Done("Goal scored or jack is lost!");
	}

	Vector center = UtilHelpers::getWorldSpaceCenter(goal);

	if (NeedsToFire())
	{
		if (bot->IsDebugging(BOTDEBUG_TASKS))
		{
			NDebugOverlay::Line(bot->GetEyeOrigin(), center, 255, 255, 255, true, 0.1f);
		}

		if (bot->GetRangeTo(center) <= MAX_RANGE_TO_LAUNCH_JACK)
		{
			Vector origin = bot->GetEyeOrigin();
			Vector aim{ center };
			aim.z += GetZ(origin, center);
			bot->GetControlInterface()->AimAt(aim, IPlayerController::LOOK_PRIORITY, 0.25f, "Aiming to throw the jack at the goal!");

			if (!m_holdattacktimer.HasStarted())
			{
				bot->GetControlInterface()->PressAttackButton(8.0f);
				m_holdattacktimer.Start(1.5f);

				if (bot->IsDebugging(BOTDEBUG_TASKS))
				{
					META_CONPRINTF("%s CTF2BotPassTimeGoalTask::OnTaskUpdate HOLDING ATTACK BUTTON! \n", bot->GetDebugIdentifier());
				}
			}
			else if (m_holdattacktimer.IsElapsed())
			{
				if (bot->GetControlInterface()->IsAimOnTarget() && bot->IsLineOfFireClear(center))
				{
					bot->GetControlInterface()->ReleaseAllAttackButtons();
				}
			}
		}
		else
		{
			m_holdattacktimer.Invalidate();
		}
	}

	if (m_repathtimer.IsElapsed())
	{
		m_repathtimer.Start(1.0f);
		CTF2BotPathCost cost(bot);
		m_nav.ComputePathToPosition(bot, center, cost, 0.0f, true);
	}

	m_nav.Update(bot);

	return Continue();
}

void CTF2BotPassTimeGoalTask::OnTaskEnd(CTF2Bot* bot, AITask<CTF2Bot>* nextTask)
{
	bot->GetControlInterface()->ReleaseAllAttackButtons();
}

bool CTF2BotPassTimeGoalTask::NeedsToFire() const
{
	switch (m_goaltype)
	{
	case TeamFortress2::TYPE_HOOP:
		return true;
	case TeamFortress2::TYPE_ENDZONE:
		return false;
	case TeamFortress2::TYPE_TOWER:
		return true;
	default:
		return false;
	}
}

float CTF2BotPassTimeGoalTask::GetZ(const Vector& origin, const Vector& center) const
{
	float range = (origin - center).Length();
	return RemapValClamped(range, 150.0f, 500.0f, 0.0f, 200.0f);
}
