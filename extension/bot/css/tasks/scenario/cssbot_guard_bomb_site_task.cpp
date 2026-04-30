#include NAVBOT_PCH_FILE
#include <mods/css/css_mod.h>
#include <bot/css/cssbot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/tasks_shared/bot_shared_defend_spot.h>
#include "cssbot_guard_bomb_site_task.h"

AITask<CCSSBot>* CCSSBotGuardBombSiteTask::InitialNextTask(CCSSBot* bot)
{
	auto settings = CCounterStrikeSourceMod::GetCSSMod()->GetCSSModSettings();
	CBaseEntity* site = UtilHelpers::GetRandomEntityOfClassname("func_bomb_target");

	if (site)
	{
		Vector pos = UtilHelpers::getWorldSpaceCenter(site);
		pos = trace::getground(pos);
		const float maxtime = CBaseBot::s_botrng.GetRandomReal<float>(settings->GetCampMinTime(), settings->GetCampMaxTime());
		return new CBotSharedDefendSpotTask<CCSSBot, CCSSBotPathCost>(bot, pos, maxtime, true);
	}

	return nullptr;
}

TaskResult<CCSSBot> CCSSBotGuardBombSiteTask::OnTaskUpdate(CCSSBot* bot)
{
	if (GetNextTask() == nullptr)
	{
		// exit and let scenario monitor pick a new behavior
		return Done("Done defending!");
	}

	if (CCounterStrikeSourceMod::GetCSSMod()->IsBombActive())
	{
		return Done("Bomb was planted!");
	}

	return Continue();
}

TaskEventResponseResult<CCSSBot> CCSSBotGuardBombSiteTask::OnBombPlanted(CCSSBot* bot, const Vector& position, const int teamIndex, CBaseEntity* player, CBaseEntity* ent)
{
	return TryDone(PRIORITY_HIGH, "Bomb was planted!");
}
