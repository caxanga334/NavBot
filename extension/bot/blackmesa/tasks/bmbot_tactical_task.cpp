#include NAVBOT_PCH_FILE
#include <extension.h>
#include <bot/blackmesa/bmbot.h>
#include <sdkports/sdk_takedamageinfo.h>
#include "bmbot_tactical_task.h"
#include "scenario/bmbot_scenario_deathmatch_task.h"
#include "bmbot_deploy_tripmines.h"
#include "bmbot_find_health_task.h"
#include <bot/tasks_shared/bot_shared_prereq_destroy_ent.h>
#include <bot/tasks_shared/bot_shared_prereq_move_to_pos.h>
#include <bot/tasks_shared/bot_shared_prereq_use_ent.h>
#include <bot/tasks_shared/bot_shared_prereq_wait.h>
#include <bot/interfaces/behavior_utils.h>
#include <bot/tasks_shared/bot_shared_pursue_and_destroy.h>

AITask<CBlackMesaBot>* CBlackMesaBotTacticalTask::InitialNextTask(CBlackMesaBot* bot)
{
	// BM is DM/TDM only
	return new CBlackMesaBotScenarioDeathmatchTask;
}

TaskResult<CBlackMesaBot> CBlackMesaBotTacticalTask::OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask)
{
	m_healthScanTimer.StartRandom(1.0f, 2.0f);
	m_deployTripminesTimer.StartRandom(5.0f, 15.0f);

	return Continue();
}

TaskResult<CBlackMesaBot> CBlackMesaBotTacticalTask::OnTaskUpdate(CBlackMesaBot* bot)
{
	if (bot->GetBehaviorInterface()->ShouldRetreat(bot) != ANSWER_NO && bot->GetBehaviorInterface()->ShouldHurry(bot) != ANSWER_YES)
	{
		if (m_healthScanTimer.IsElapsed())
		{
			m_healthScanTimer.StartRandom(1.0f, 2.0f);

			if (CBlackMesaBotFindHealthTask::IsPossible(bot))
			{
				return PauseFor(new CBlackMesaBotFindHealthTask, "Searching for health!");
			}
		}
	}

	if (m_deployTripminesTimer.IsElapsed())
	{
		m_deployTripminesTimer.StartRandom(5.0f, 15.0f);

		Vector pos;
		if (CBlackMesaBotDeployTripminesTask::IsPossible(bot, pos))
		{
			return PauseFor(new CBlackMesaBotDeployTripminesTask(pos), "Opportunistically deploying tripmines!");
		}
	}

	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (threat && bot->GetBehaviorInterface()->ShouldSeekAndDestroy(bot, threat) != ANSWER_NO)
	{
		return PauseFor(new CBotSharedPursueAndDestroyTask(bot, threat->GetEntity()), "Pursuing visible threat!");
	}

	return Continue();
}

TaskResult<CBlackMesaBot> CBlackMesaBotTacticalTask::OnTaskResume(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask)
{
	m_healthScanTimer.StartRandom(1.0f, 2.0f);
	m_deployTripminesTimer.StartRandom(5.0f, 15.0f);

	return Continue();
}

QueryAnswerType CBlackMesaBotTacticalTask::ShouldSeekAndDestroy(CBaseBot* baseBot, const CKnownEntity* them)
{
	CBlackMesaBot* me = static_cast<CBlackMesaBot*>(baseBot);

	if (me->GetHealthPercentage() > 0.95 && me->GetArmorPercentage() >= 0.80)
	{
		return ANSWER_YES;
	}

	return ANSWER_NO;
}

TaskEventResponseResult<CBlackMesaBot> CBlackMesaBotTacticalTask::OnNavAreaChanged(CBlackMesaBot* bot, CNavArea* oldArea, CNavArea* newArea)
{
	BOTBEHAVIOR_IMPLEMENT_PREREQUISITE_CHECK(CBlackMesaBot, CBlackMesaBotPathCost);

	return TryContinue();
}
