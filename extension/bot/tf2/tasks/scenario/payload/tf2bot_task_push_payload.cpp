#include <extension.h>
#include <util/entprops.h>
#include <mods/tf2/teamfortress2mod.h>
#include <bot/tf2/tf2bot.h>
#include "tf2bot_task_push_payload.h"

TaskResult<CTF2Bot> CTF2BotPushPayloadTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* payload;

	if (bot->GetMyTFTeam() == TeamFortress2::TFTeam_Red)
	{
		payload = CTeamFortress2Mod::GetTF2Mod()->GetREDPayload();
	}
	else
	{
		payload = CTeamFortress2Mod::GetTF2Mod()->GetBLUPayload();
	}

	m_payload.Set(payload);

	m_repathtimer.Start(0.4f);
	
	if (payload != nullptr)
	{
		m_updatePayloadTimer.Start(10.0f);
	}
	else
	{
		m_updatePayloadTimer.Start(0.1f);
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotPushPayloadTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (entprops->GameRules_GetRoundState() != RoundState_RoundRunning)
	{
		return Continue();
	}

	CBaseEntity* payload = GetPayload(bot);

	if (payload == nullptr)
	{
		return Continue();
	}

	if (!m_nav.IsValid() || m_repathtimer.IsElapsed())
	{
		m_goal = UtilHelpers::getWorldSpaceCenter(m_payload.ToEdict());

		CTF2BotPathCost cost(bot);
		m_nav.ComputePathToPosition(bot, m_goal, cost);

		m_repathtimer.Start(0.4f);
	}

	m_nav.Update(bot);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotPushPayloadTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* payload;

	if (bot->GetMyTFTeam() == TeamFortress2::TFTeam_Red)
	{
		payload = CTeamFortress2Mod::GetTF2Mod()->GetREDPayload();
	}
	else
	{
		payload = CTeamFortress2Mod::GetTF2Mod()->GetBLUPayload();
	}

	m_payload.Set(payload);

	if (payload != nullptr)
	{
		m_updatePayloadTimer.Start(10.0f);
	}
	else
	{
		m_updatePayloadTimer.Start(0.1f);
	}

	m_nav.Invalidate();

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotPushPayloadTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	m_nav.Invalidate();
	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotPushPayloadTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	return TryContinue();
}

CBaseEntity* CTF2BotPushPayloadTask::GetPayload(CTF2Bot* bot)
{
	CBaseEntity* payload = m_payload.Get();

	// some maps changes the target payload entity so we need to update from time to time.
	// this is generally not needed unless since the bot behavior is reset when they are killed but this is for edge cases where a bot manages to stay alive
	if (payload == nullptr || m_updatePayloadTimer.IsElapsed())
	{
		if (bot->GetMyTFTeam() == TeamFortress2::TFTeam_Red)
		{
			payload = CTeamFortress2Mod::GetTF2Mod()->GetREDPayload();
		}
		else
		{
			payload = CTeamFortress2Mod::GetTF2Mod()->GetBLUPayload();
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
