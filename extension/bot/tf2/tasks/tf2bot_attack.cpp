#include <algorithm>

#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include "tf2bot_attack.h"

CTF2BotAttackTask::CTF2BotAttackTask(CBaseEntity* entity, const float escapeTime, const float maxChaseTime)
{
	m_target = entity;
	m_chaseDuration = maxChaseTime;
	m_escapeDuration = escapeTime;
}

TaskResult<CTF2Bot> CTF2BotAttackTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (m_target.Get() == nullptr)
	{
		return Done("NULL target entity!");
	}

	m_expireTimer.Start(m_chaseDuration);
	m_escapeTimer.Start(m_escapeDuration);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotAttackTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_expireTimer.IsElapsed())
	{
		return Done("This is taking too much time! Giving up!");
	}

	if (bot->GetHealthPercentage() <= 0.30f)
	{
		return Done("Backing off, low health!");
	}

	CBaseEntity* pEntity = m_target.Get();

	if (pEntity == nullptr)
	{
		return Done("Target is NULL!");
	}

	int index = gamehelpers->EntityToBCompatRef(m_target.Get());

	if (!UtilHelpers::IsEntityAlive(index))
	{
		return Done("Target is dead!");
	}

	auto known = bot->GetSensorInterface()->GetKnown(pEntity);
	auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	// if sensor lost track of it, it's gone.
	if (known.get() == nullptr)
	{
		return Done("Target has escaped me!");
	}

	// don't get too close to them
	if (!bot->GetSensorInterface()->IsAbleToSee(pEntity) || bot->GetRangeTo(pEntity) > 250.0f)
	{
		CTF2BotPathCost cost(bot);
		m_nav.Update(bot, pEntity, cost, nullptr);
	}

	if (threat && known && threat.get() == known.get())
	{
		// don't use combat look priority in case another more important threat is visible to us
		bot->GetControlInterface()->AimAt(pEntity, IPlayerController::LOOK_DANGER, 0.25f, "Looking at target entity!");
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotAttackTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* pEntity = m_target.Get();

	if (pEntity == nullptr)
	{
		return Done("Target is NULL!");
	}

	if (!bot->GetSensorInterface()->IsAbleToSee(pEntity))
	{
		return Done("Target has escaped me!");
	}

	return Continue();
}
