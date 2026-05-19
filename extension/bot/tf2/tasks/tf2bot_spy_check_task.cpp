#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/interfaces/path/chasenavigator.h>
#include "tf2bot_spy_check_task.h"

bool CTF2BotSpyCheckTask::IsPossible(CTF2Bot* bot, CBaseEntity* spy)
{
	switch (bot->GetMyClassType())
	{
	case TeamFortress2::TFClassType::TFClass_Medic:
		[[fallthrough]];
	case TeamFortress2::TFClassType::TFClass_Engineer:
		return false;
	default:
		break;
	}

	// low skill bots don't spy check
	if (bot->GetDifficultyProfile()->GetGameAwareness() < 60)
	{
		return false;
	}

	constexpr float MAX_SPY_CHECK_RANGE = 750.0f * 750.0f;

	if (bot->GetRangeToSqr(spy) > MAX_SPY_CHECK_RANGE)
	{
		return false;
	}

	if (!modhelpers->IsPlayer(spy))
	{
		return false;
	}

	if (tf2lib::GetPlayerClassType(UtilHelpers::IndexOfEntity(spy)) != TeamFortress2::TFClassType::TFClass_Spy)
	{
		return false;
	}

	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ONLY_VISIBLE_THREATS);

	// distracted attacking another enemy
	if (threat)
	{
		return false;
	}

	// don't spycheck spies from my own team.
	if (bot->GetSensorInterface()->IsFriendly(spy))
	{
		return false;
	}

	auto& known = bot->GetSpyMonitorInterface()->GetKnownSpy(spy);

	// already spotted it
	if (known.IsHostile())
	{
		return false;
	}

	return true;
}

CTF2BotSpyCheckTask::CTF2BotSpyCheckTask(CBaseEntity* target) :
	m_nav(CChaseNavigator::SubjectLeadType::DONT_LEAD_SUBJECT), m_target(target), m_pto()
{
}

TaskResult<CTF2Bot> CTF2BotSpyCheckTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* spy = m_target.Get();

	if (!spy)
	{
		return Done("Spy is NULL!");
	}

	m_timeout.StartRandom(3.0f, 5.0f);
	m_pto.Set(bot, spy);
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSpyCheckTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* spy = m_target.Get();

	if (!spy)
	{
		return Done("Spy is NULL!");
	}

	if (modhelpers->IsDead(spy))
	{
		return Done("Spy is dead!");
	}

	auto& known = bot->GetSpyMonitorInterface()->GetKnownSpy(spy);

	// already spotted it
	if (known.IsHostile())
	{
		return Done("Spy was revealed!");
	}

	if (m_timeout.IsElapsed())
	{
		return Done("Timeout timer elapsed!");
	}

	CTF2BotPathCost cost{ bot, RouteType::FASTEST_ROUTE };
	m_nav.Update(bot, spy, cost);

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotSpyCheckTask::OnSight(CTF2Bot* bot, CBaseEntity* subject)
{
	return TryToMaintain(PRIORITY_MEDIUM);
}

TaskEventResponseResult<CTF2Bot> CTF2BotSpyCheckTask::OnLostSight(CTF2Bot* bot, CBaseEntity* subject)
{
	CBaseEntity* spy = m_target.Get();

	if (spy == subject)
	{
		return TryDone(PRIORITY_HIGH, "Spy eluded me!");
	}

	return TryToMaintain(PRIORITY_MEDIUM);
}
