#include <extension.h>
#include <mods/basemod.h>
#include <mods/tf2/teamfortress2mod.h>
#include <entities/tf2/tf_entities.h>
#include "bot/tf2/tf2bot.h"
#include <bot/tf2/tasks/scenario/tf2bot_map_ctf.h>
#include <bot/tf2/tasks/medic/tf2bot_medic_main_task.h>
#include "tf2bot_collect_item.h"

CTF2BotCollectItemTask::CTF2BotCollectItemTask(CBaseEntity* item, const bool failOnPause, const bool ignoreExisting) :
	m_failOnPause(failOnPause), m_ignoreExisting(ignoreExisting), m_name("CollectItem"), m_moveFailures(0)
{
}

TaskResult<CTF2Bot> CTF2BotCollectItemTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (m_validationFunc)
	{
		bool result = m_validationFunc(bot, m_item.Get());

		if (!result)
		{
			return Done("Item is invalid!");
		}
	}
	else
	{
		if (!DefaultValidator(bot))
		{
			return Done("Item is invalid!");
		}
	}

	if (m_positionFunc)
	{
		m_moveGoal = m_positionFunc(bot, m_item.Get());
	}
	else
	{
		tfentities::HTFBaseEntity BE(m_item.Get());
		m_moveGoal = BE.WorldSpaceCenter();
	}

	CTF2BotPathCost cost(bot);
	
	if (!m_nav.ComputePathToPosition(bot, m_moveGoal, cost))
	{
		return Done("Failed to build a path to the item!");
	}

	m_repathTimer.Start(3.0f);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotCollectItemTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_validationFunc)
	{
		bool result = m_validationFunc(bot, m_item.Get());

		if (!result)
		{
			return Done("Item is invalid!");
		}
	}
	else
	{
		if (!DefaultValidator(bot))
		{
			return Done("Item is invalid!");
		}
	}

	if (m_goalFunc)
	{
		bool result = m_goalFunc(bot, m_item.Get());

		if (result)
		{
			return Done("Task goal completed!");
		}
	}
	else
	{
		if (bot->GetItem() != nullptr)
		{
			return Done("I have an item! Task completed.");
		}
	}

	if (m_repathTimer.IsElapsed())
	{
		if (m_positionFunc)
		{
			m_moveGoal = m_positionFunc(bot, m_item.Get());
		}
		else
		{
			tfentities::HTFBaseEntity BE(m_item.Get());
			m_moveGoal = BE.WorldSpaceCenter();
		}

		CTF2BotPathCost cost(bot);

		if (!m_nav.ComputePathToPosition(bot, m_moveGoal, cost))
		{
			return Done("Failed to build a path to the item!");
		}

		m_repathTimer.Start(3.0f);
	}

	m_nav.Update(bot);

	return Continue();
}

bool CTF2BotCollectItemTask::OnTaskPause(CTF2Bot* bot, AITask<CTF2Bot>* nextTask)
{
	// If fail on pause is set
	if (m_failOnPause)
	{
		// we return false here, the task manager will destroy this task
		return false;
	}

	// otherwise we return true and the task is kept paused until the the task that paused this one ends
	return true;
}

TaskResult<CTF2Bot> CTF2BotCollectItemTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (m_validationFunc)
	{
		bool result = m_validationFunc(bot, m_item.Get());

		if (!result)
		{
			return Done("Item is invalid!");
		}
	}
	else
	{
		if (!DefaultValidator(bot))
		{
			return Done("Item is invalid!");
		}
	}

	if (m_goalFunc)
	{
		bool result = m_goalFunc(bot, m_item.Get());

		if (result)
		{
			return Done("Task goal completed!");
		}
	}
	else
	{
		if (bot->GetItem() != nullptr)
		{
			return Done("I have an item! Task completed.");
		}
	}

	m_nav.Invalidate();

	if (m_positionFunc)
	{
		m_moveGoal = m_positionFunc(bot, m_item.Get());
	}
	else
	{
		tfentities::HTFBaseEntity BE(m_item.Get());
		m_moveGoal = BE.WorldSpaceCenter();
	}

	CTF2BotPathCost cost(bot);

	if (!m_nav.ComputePathToPosition(bot, m_moveGoal, cost))
	{
		return Done("Failed to build a path to the item!");
	}

	m_repathTimer.Start(3.0f);

	if (m_moveFailures > 10)
	{
		m_moveFailures = 10; // Reduce a bit
	}

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotCollectItemTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	if (++m_moveFailures >= 20)
	{
		return TryDone(PRIORITY_MEDIUM, "Too many pathing failures, aborting!");
	}

	m_nav.Invalidate();

	if (m_positionFunc)
	{
		m_moveGoal = m_positionFunc(bot, m_item.Get());
	}
	else
	{
		tfentities::HTFBaseEntity BE(m_item.Get());
		m_moveGoal = BE.WorldSpaceCenter();
	}

	CTF2BotPathCost cost(bot);

	if (!m_nav.ComputePathToPosition(bot, m_moveGoal, cost))
	{
		return TryDone(PRIORITY_HIGH, "Failed to build a path to the item!");
	}

	return TryContinue(PRIORITY_LOW);
}

TaskEventResponseResult<CTF2Bot> CTF2BotCollectItemTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	if (m_validationFunc)
	{
		bool result = m_validationFunc(bot, m_item.Get());

		if (!result)
		{
			return TryDone(PRIORITY_HIGH, "Item is invalid!");
		}
	}
	else
	{
		if (!DefaultValidator(bot))
		{
			return TryDone(PRIORITY_HIGH, "Item is invalid!");
		}
	}

	if (m_goalFunc)
	{
		bool result = m_goalFunc(bot, m_item.Get());

		if (result)
		{
			return TryDone(PRIORITY_HIGH, "Task goal completed!");
		}
	}
	else
	{
		if (bot->GetItem() != nullptr)
		{
			return TryDone(PRIORITY_HIGH, "I have an item! Task completed.");
		}
	}

	return TryContinue(PRIORITY_LOW);
}

bool CTF2BotCollectItemTask::DefaultValidator(CTF2Bot* bot)
{
	if (m_item.Get() == nullptr)
	{
		return false;
	}

	if (!m_ignoreExisting && bot->GetItem() != nullptr)
	{
		return false;
	}

	return true;
}
