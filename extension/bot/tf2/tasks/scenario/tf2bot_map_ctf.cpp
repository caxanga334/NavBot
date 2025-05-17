#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <util/librandom.h>
#include <sdkports/sdk_timers.h>
#include <sdkports/sdk_traces.h>
#include <entities/tf2/tf_entities.h>
#include "bot/tf2/tf2bot.h"
#include <bot/tf2/tasks/tf2bot_roam.h>
#include <mods/tf2/teamfortress2mod.h>
#include "tf2bot_map_ctf.h"

TaskResult<CTF2Bot> CTF2BotCTFMonitorTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (bot->IsCarryingAFlag())
	{
		return PauseFor(new CTF2BotCTFDeliverFlagTask, "Going to deliver the flag!");
	}
	else
	{
		// get dropped and stolen flags
		edict_t* flag_to_defend = bot->GetFlagToDefend(false, true);
		edict_t* flag = bot->GetFlagToFetch();
		int defrate = CTeamFortress2Mod::GetTF2Mod()->GetModSettings()->GetDefendRate();

		if (flag != nullptr && flag_to_defend != nullptr)
		{
			// I have both a flag to attack and defend
			if (randomgen->GetRandomInt<int>(0, 100) < defrate)
			{
				CBaseEntity* ent = flag_to_defend->GetIServerEntity()->GetBaseEntity();
				return PauseFor(new CTF2BotCTFDefendFlag(ent), "Defending a dropped/stolen flag!");
			}
			else
			{
				return PauseFor(new CTF2BotCTFFetchFlagTask(flag), "Going to fetch the flag!");
			}
		}
		else if (flag == nullptr && flag_to_defend != nullptr)
		{
			CBaseEntity* ent = flag_to_defend->GetIServerEntity()->GetBaseEntity();
			return PauseFor(new CTF2BotCTFDefendFlag(ent), "Defending a dropped/stolen flag!");
		}
	
		if (flag != nullptr)
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

CTF2BotCTFFetchFlagTask::CTF2BotCTFFetchFlagTask(edict_t* flag) :
	m_goalpos(0.0f, 0.0f, 0.0f), m_routetype(DEFAULT_ROUTE)
{
	gamehelpers->SetHandleEntity(m_flag, flag);
}

TaskResult<CTF2Bot> CTF2BotCTFFetchFlagTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	auto edict = gamehelpers->GetHandleEntity(m_flag);

	if (!edict)
		return Done("Flag is NULL!");

	if (bot->GetDifficultyProfile()->GetAggressiveness() >= 50)
	{
		m_routetype = static_cast<RouteType>(randomgen->GetRandomInt<int>(1, 2));
	}
	else
	{
		m_routetype = SAFEST_ROUTE;
	}

	tfentities::HCaptureFlag flag(edict);

	m_goalpos = flag.GetPosition();

	CTF2BotPathCost cost(bot, m_routetype);
	
	if (!m_nav.ComputePathToPosition(bot, m_goalpos, cost))
	{
		return Done("Failed to find a path to the flag!");
	}

	m_repathtimer.StartRandom(1.0f, 1.5f);



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

		CTF2BotPathCost cost(bot, m_routetype);
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
	m_capzone = UtilHelpers::EdictToBaseEntity(goalzone);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotCTFDeliverFlagTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (!bot->IsCarryingAFlag())
	{
		return Done("Flag delivered!");
	}

	CBaseEntity* capzone = m_capzone.Get();

	if (!capzone)
	{
		return Done("Capture zone is NULL!");
	}

	if (m_repathtimer.IsElapsed())
	{
		m_repathtimer.Start(1.0f);
		// refresh pos, some maps have the capture zone parented to a moving entity
		m_goalpos = UtilHelpers::getWorldSpaceCenter(capzone);
		m_goalpos = trace::getground(m_goalpos);

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

	return TryContinue();
}

CTF2BotCTFDefendFlag::CTF2BotCTFDefendFlag(CBaseEntity* flag)
{
	m_flag = flag;
}

TaskResult<CTF2Bot> CTF2BotCTFDefendFlag::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (m_flag.Get() == nullptr)
	{
		return Done("Flag is invalid!");
	}

	m_giveupTimer.StartRandom(90.0f, 180.0f);
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotCTFDefendFlag::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_giveupTimer.IsElapsed())
	{
		return Done("Defend timer elapsed!");
	}

	CBaseEntity* ent = m_flag.Get();

	if (ent == nullptr)
	{
		return Done("Flag is invalid!");
	}

	tfentities::HCaptureFlag flag(ent);

	if (flag.IsHome())
	{
		return Done("Flag returned home!");
	}
	
	// if the flag is stolen, this will return the player's position
	Vector pos = flag.GetPosition();
	
	if (bot->GetRangeTo(pos) > 512.0f || !bot->GetSensorInterface()->IsAbleToSee(pos))
	{
		CTF2BotPathCost cost(bot);
		m_nav.Update(bot, pos, cost); // go after the flag
	}

	return Continue();
}
