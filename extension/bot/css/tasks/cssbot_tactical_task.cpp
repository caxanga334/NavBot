#include NAVBOT_PCH_FILE
#include <mods/css/css_lib.h>
#include <mods/css/css_mod.h>
#include <bot/css/cssbot.h>
#include <bot/bot_shared_utils.h>
#include "cssbot_buy_weapons_task.h"
#include "cssbot_scenario_task.h"
#include "cssbot_tactical_task.h"

AITask<CCSSBot>* CCSSBotTacticalTask::InitialNextTask(CCSSBot* bot)
{
	return new CCSSBotScenarioTask;
}

TaskResult<CCSSBot> CCSSBotTacticalTask::OnTaskStart(CCSSBot* bot, AITask<CCSSBot>* pastTask)
{
	if (csslib::IsInFreezeTime())
	{
		return SwitchTo(new CCSSBotBuyWeaponsTask, "Freeze time!");
	}

	return Continue();
}

TaskResult<CCSSBot> CCSSBotTacticalTask::OnTaskUpdate(CCSSBot* bot)
{
	return Continue();
}

TaskEventResponseResult<CCSSBot> CCSSBotTacticalTask::OnKilled(CCSSBot* bot, const CTakeDamageInfo& info)
{
	botsharedutils::SpreadDangerToNearbyAreas func{ bot->GetLastKnownNavArea(), static_cast<int>(bot->GetMyCSSTeam()), 1024.0f, CNavArea::ADD_DANGER_KILLED, CNavArea::MAX_DANGER_ONKILLED };

	func.Execute();

	return TryContinue(PRIORITY_LOW);
}

QueryAnswerType CCSSBotTacticalTask::ShouldHurry(CBaseBot* me)
{
	if (csslib::GetRoundTimeRemaining() <= CCounterStrikeSourceMod::GetCSSMod()->GetCSSModSettings()->GetTimeLeftToHurry())
	{
		return ANSWER_YES;
	}

	return ANSWER_UNDEFINED;
}
