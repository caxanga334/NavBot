#include NAVBOT_PCH_FILE
#include <extension.h>
#include <mods/tf2/teamfortress2mod.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tasks_shared/bot_shared_attack_enemy.h>
#include <bot/tasks_shared/bot_shared_search_area.h>
#include <bot/tf2/tasks/demoman/tf2bot_demoman_lay_sticky_trap_task.h>
#include "tf2bot_task_defend_payload.h"

TaskResult<CTF2Bot> CTF2BotDefendPayloadTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	GetPayload(bot); // update payload
	// if true, will stay near the payload, otherwise will patrol around
	m_defendPayload = CTeamFortress2Mod::GetTF2Mod()->GetTF2ModSettings()->RollDefendChance();

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotDefendPayloadTask::OnTaskUpdate(CTF2Bot* bot)
{
	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ONLY_VISIBLE_THREATS);

	if (threat)
	{
		return PauseFor(new CBotSharedAttackEnemyTask<CTF2Bot, CTF2BotPathCost>(bot, CBaseBot::s_botrng.GetRandomReal<float>(3.0f, 6.0f)), "Attacking visible threat!");
	}

	CBaseEntity* payload = GetPayload(bot);

	if (payload == nullptr)
	{
		return Continue();
	}

	const Vector center = UtilHelpers::getWorldSpaceCenter(payload);
	CTraceFilterWorldAndPropsOnly filter;
	trace_t tr;
	trace::line(bot->GetEyeOrigin(), center, MASK_BLOCKLOS, tr);

	if (bot->GetRangeToSqr(center) > DEFEND_PAYLOAD_RANGE || (!tr.startsolid && tr.fraction == 1.0f))
	{
		if (!m_nav.IsValid() || m_nav.NeedsRepath())
		{
			m_nav.StartRepathTimer();

			CTF2BotPathCost cost(bot);
			m_nav.ComputePathToPosition(bot, center, cost);
		}

		m_nav.Update(bot);
	}
	else
	{
		if (CTF2BotDemomanLayStickyTrapTask::IsPossible(bot) && CBaseBot::s_botrng.GetRandomChance(50))
		{
			return PauseFor(new CTF2BotDemomanLayStickyTrapTask(center), "Deploying sticky trap!");
		}

		if (!m_defendPayload)
		{
			return PauseFor(new CBotSharedSearchAreaTask<CTF2Bot, CTF2BotPathCost>(bot, bot->GetAbsOrigin(), 512.0f, 2048.0f, 32U), "Patrolling area near payload for enemies.");
		}
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotDefendPayloadTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	GetPayload(bot); // update payload
	m_nav.Invalidate();
	m_nav.ForceRepath();

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotDefendPayloadTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotDefendPayloadTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	return TryContinue();
}

CBaseEntity* CTF2BotDefendPayloadTask::GetPayload(CTF2Bot* bot)
{
	CBaseEntity* payload = m_payload.Get();

	// some maps changes the target payload entity so we need to update from time to time.
	// this is generally not needed unless since the bot behavior is reset when they are killed but this is for edge cases where a bot manages to stay alive
	if (payload == nullptr || m_updatePayloadTimer.IsElapsed())
	{
		if (bot->GetMyTFTeam() == TeamFortress2::TFTeam_Red)
		{
			payload = CTeamFortress2Mod::GetTF2Mod()->GetBLUPayload();
		}
		else
		{
			payload = CTeamFortress2Mod::GetTF2Mod()->GetREDPayload();
		}

		if (payload == nullptr)
		{
			m_updatePayloadTimer.Start(0.1f);
		}
		else
		{
			m_updatePayloadTimer.Start(10.0f);
		}

		m_payload.Set(payload);
	}

	return payload;
}
