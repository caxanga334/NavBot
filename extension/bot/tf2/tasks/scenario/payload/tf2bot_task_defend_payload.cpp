#include NAVBOT_PCH_FILE
#include <extension.h>
#include <mods/tf2/teamfortress2mod.h>
#include <bot/tf2/tf2bot.h>
#include "tf2bot_task_defend_payload.h"

TaskResult<CTF2Bot> CTF2BotDefendPayloadTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	GetPayload(bot); // update payload
	m_repathtimer.Invalidate();

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotDefendPayloadTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* payload = GetPayload(bot);

	if (payload == nullptr)
	{
		return Continue();
	}

	if (bot->GetRangeToSqr(payload) > 300.0f)
	{
		if (!m_nav.IsValid() || m_repathtimer.IsElapsed())
		{
			m_repathtimer.Start(0.8f);

			CTF2BotPathCost cost(bot);
			Vector pos = UtilHelpers::getWorldSpaceCenter(payload);
			m_nav.ComputePathToPosition(bot, pos, cost);
		}

		m_nav.Update(bot);
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotDefendPayloadTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	GetPayload(bot); // update payload
	m_repathtimer.Invalidate();

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
