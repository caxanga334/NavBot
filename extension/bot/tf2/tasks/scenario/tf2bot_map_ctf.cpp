#include <extension.h>
#include <util/helpers.h>
#include <sdkports/sdk_timers.h>
#include <sdkports/sdk_traces.h>
#include <entities/tf2/tf_entities.h>
#include "bot/tf2/tf2bot.h"
#include <bot/tf2/tasks/tf2bot_roam.h>
#include "tf2bot_map_ctf.h"

TaskResult<CTF2Bot> CTF2BotCTFMonitorTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (bot->IsCarryingAFlag())
	{
		return PauseFor(new CTF2BotCTFDeliverFlagTask, "Going to deliver the flag!");
	}
	else
	{
		auto flag = bot->GetFlagToFetch();

		if (flag)
		{
			return PauseFor(new CTF2BotCTFFetchFlagTask(flag), "Going to fetch the flag!");
		}
	}

	if (randomgen->GetRandomInt<int>(0, 1) == 1)
	{
		return PauseFor(new CTF2BotRoamTask(), "No flag to deliver or fetch, roaming!");
	}

	return Continue();
}

CTF2BotCTFFetchFlagTask::CTF2BotCTFFetchFlagTask(edict_t* flag)
{
	gamehelpers->SetHandleEntity(m_flag, flag);
}

TaskResult<CTF2Bot> CTF2BotCTFFetchFlagTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	auto edict = gamehelpers->GetHandleEntity(m_flag);

	if (!edict)
		return Done("Flag is NULL!");

	tfentities::HCaptureFlag flag(edict);

	m_goalpos = flag.GetPosition();

	CTF2BotPathCost cost(bot);
	
	if (!m_nav.ComputePathToPosition(bot, m_goalpos, cost))
	{
		return Done("Failed to find a path to the flag!");
	}

	m_repathtimer.Start(0.5f);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotCTFFetchFlagTask::OnTaskUpdate(CTF2Bot* bot)
{
	auto edict = gamehelpers->GetHandleEntity(m_flag);

	if (!edict)
		return Done("Flag is NULL!");

	tfentities::HCaptureFlag flag(edict);

	if (flag.IsStolen())
	{
		if (gamehelpers->IndexOfEdict(edict) == gamehelpers->IndexOfEdict(bot->GetItem()))
		{
			return SwitchTo(new CTF2BotCTFDeliverFlagTask, "I have the flag, going to deliver it!");
		}

		return Done("Someone stole the flag before me!");
	}

	if (m_repathtimer.IsElapsed())
	{
		m_repathtimer.Reset();
		m_goalpos = flag.GetPosition();

		CTF2BotPathCost cost(bot);
		if (!m_nav.ComputePathToPosition(bot, m_goalpos, cost))
		{
			return Done("Failed to find a path to the flag!");
		}
	}

	m_nav.Update(bot);
	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotCTFFetchFlagTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	CTF2BotPathCost cost(bot);

	if (!m_nav.ComputePathToPosition(bot, m_goalpos, cost))
	{
		return TryDone(PRIORITY_HIGH, "Failed to find a path to the flag!");
	}

	m_repathtimer.Start(0.5f);
	bot->GetMovementInterface()->ClearStuckStatus();

	return TryContinue();
}

TaskResult<CTF2Bot> CTF2BotCTFDeliverFlagTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	auto goalzone = bot->GetFlagCaptureZoreToDeliver();

	if (!goalzone)
	{
		return Done("Failed to find a capture zone!");
	}

	Vector center = UtilHelpers::getWorldSpaceCenter(goalzone);

	CTraceFilterWorldAndPropsOnly filter;
	trace_t result;
	// get the ground position
	trace::line(center, center + Vector(0.0f, 0.0f, -2048.0f), MASK_PLAYERSOLID_BRUSHONLY, bot->GetEntity(), COLLISION_GROUP_PLAYER_MOVEMENT, result);

	if (!result.DidHit())
	{
		return Done("Capture zone is floating in the air???");
	}

	Vector goal = result.endpos;
	goal.z += 3.0f;

	m_goalpos = goal;

	CTF2BotPathCost cost(bot);

	if (!m_nav.ComputePathToPosition(bot, goal, cost))
	{
		return Done("Failed to find a path to the capture zone!");
	}

	m_repathtimer.Start(0.5f);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotCTFDeliverFlagTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (!bot->IsCarryingAFlag())
	{
		return Done("Flag delivered!");
	}

	if (m_repathtimer.IsElapsed())
	{
		m_repathtimer.Start(0.5f);

		CTF2BotPathCost cost(bot);

		if (!m_nav.ComputePathToPosition(bot, m_goalpos, cost))
		{
			return Done("Failed to find a path to the capture zone!");
		}
	}

	m_nav.Update(bot);

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotCTFDeliverFlagTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	CTF2BotPathCost cost(bot);

	if (!m_nav.ComputePathToPosition(bot, m_goalpos, cost))
	{
		return TryDone(PRIORITY_HIGH, "Failed to find a path to the capture zone!");
	}

	m_repathtimer.Start(0.5f);
	bot->GetMovementInterface()->ClearStuckStatus();

	return TryContinue();
}