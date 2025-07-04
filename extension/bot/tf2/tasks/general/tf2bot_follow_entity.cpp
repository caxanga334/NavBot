#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <mods/basemod.h>
#include <mods/tf2/teamfortress2mod.h>
#include <entities/tf2/tf_entities.h>
#include "bot/tf2/tf2bot.h"
#include "tf2bot_follow_entity.h"

CTF2BotFollowEntityTask::CTF2BotFollowEntityTask(CBaseEntity* entity, const float updateRate, const float followRange, const bool failOnPause) :
	m_followtarget(entity), m_updaterate(updateRate), m_followrange(followRange), m_failOnPause(failOnPause), m_pathfailures(0)
{
	m_updateEntityPositionTimer.Start(updateRate);
	auto classname = gamehelpers->GetEntityClassname(entity);

	if (classname != nullptr && classname[0] != '\0')
	{
		char actionname[256]{};
		ke::SafeSprintf(actionname, sizeof(actionname), "FollowEntity<%s>", classname);
		m_name.assign(actionname);
	}
	else
	{
		m_name.assign("FollowEntity");
	}

	m_isplayer = UtilHelpers::IsPlayerIndex(gamehelpers->EntityToBCompatRef(entity));
}

TaskResult<CTF2Bot> CTF2BotFollowEntityTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (!ValidateFollowEntity(bot))
	{
		return Done("Follow entity is invalid!");
	}

	GetEntityPosition(bot, m_followgoal);

	CTF2BotPathCost cost(bot);
	if (!m_nav.ComputePathToPosition(bot, m_followgoal, cost))
	{
		return Done("Failed to build a path to the follow entity!");
	}

#ifdef EXT_DEBUG
	if (!(m_goalFunc))
	{
		Warning("%s CTF2BotFollowEntityTask without m_goalFunc set! The bot will follow the entity until it no longer exists!\n", bot->GetDebugIdentifier());
	}
#endif // EXT_DEBUG

	m_updateEntityPositionTimer.Start(m_updaterate);
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotFollowEntityTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (!ValidateFollowEntity(bot))
	{
		return Done("Follow entity is invalid!");
	}

	if (m_updateEntityPositionTimer.IsElapsed())
	{
		GetEntityPosition(bot, m_followgoal);
		m_updateEntityPositionTimer.Start(m_updaterate);

		CTF2BotPathCost cost(bot);
		if (!m_nav.ComputePathToPosition(bot, m_followgoal, cost))
		{
			return Done("Failed to build a path to the follow entity!");
		}
	}

	const float range = bot->GetRangeTo(m_followgoal);

	if (range > m_followrange)
	{
		m_nav.Update(bot);
	}

	if (m_goalFunc)
	{
		if (m_goalFunc(bot, m_followtarget.Get()))
		{
			return Done("Follow goal reached!");
		}
	}

	return Continue();
}

bool CTF2BotFollowEntityTask::OnTaskPause(CTF2Bot* bot, AITask<CTF2Bot>* nextTask)
{
	// must return true to keep it running
	return !m_failOnPause;
}

TaskResult<CTF2Bot> CTF2BotFollowEntityTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (!ValidateFollowEntity(bot))
	{
		return Done("Follow entity is invalid!");
	}

	GetEntityPosition(bot, m_followgoal);

	CTF2BotPathCost cost(bot);
	if (!m_nav.ComputePathToPosition(bot, m_followgoal, cost))
	{
		return Done("Failed to build a path to the follow entity!");
	}

	if (m_goalFunc)
	{
		if (m_goalFunc(bot, m_followtarget.Get()))
		{
			return Done("Follow goal reached!");
		}
	}

	if (m_pathfailures > 10)
	{
		m_pathfailures = 10;
	}

	m_updateEntityPositionTimer.Start(m_updaterate);
	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotFollowEntityTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	if (++m_pathfailures > 20)
	{
		return TryDone(PRIORITY_HIGH, "Too many path failures!");
	}

	if (!ValidateFollowEntity(bot))
	{
		return TryDone(PRIORITY_HIGH, "Entity is no longer valid!");
	}

	GetEntityPosition(bot, m_followgoal);

	CTF2BotPathCost cost(bot);
	if (!m_nav.ComputePathToPosition(bot, m_followgoal, cost))
	{
		return TryDone(PRIORITY_HIGH, "Failed to build a path to the follow entity!");
	}

	m_updateEntityPositionTimer.Start(m_updaterate);

	return TryContinue(PRIORITY_LOW);
}

TaskEventResponseResult<CTF2Bot> CTF2BotFollowEntityTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	if (m_goalFunc)
	{
		if (m_goalFunc(bot, m_followtarget.Get()))
		{
			return TryDone(PRIORITY_HIGH, "Goal entity reached!");
		}
	}

	return TryContinue(PRIORITY_LOW);
}

bool CTF2BotFollowEntityTask::ValidateFollowEntity(CTF2Bot* me)
{
	CBaseEntity* target = m_followtarget.Get();

	// Target is no longer valid
	if (target == nullptr)
	{
		return false;
	}

	// Stop following dead players
	if (m_isplayer && !UtilHelpers::IsPlayerAlive(gamehelpers->EntityToBCompatRef(target)))
	{
		return false;
	}

	if (m_validationFunc)
	{
		if (!m_validationFunc(me, target))
		{
			return false;
		}
	}

	return true;
}

void CTF2BotFollowEntityTask::GetEntityPosition(CTF2Bot* me, Vector& out)
{
	if (m_positionFunc)
	{
		out = m_positionFunc(me, m_followtarget.Get());
	}
	else
	{
		tfentities::HTFBaseEntity ent(m_followtarget.Get());

		if (m_isplayer)
		{
			out = ent.GetAbsOrigin();
		}
		else
		{
			out = ent.WorldSpaceCenter();
		}
	}
}


