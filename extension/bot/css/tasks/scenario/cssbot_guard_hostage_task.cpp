#include NAVBOT_PCH_FILE
#include <mods/css/css_mod.h>
#include <bot/css/cssbot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/tasks_shared/bot_shared_defend_spot.h>
#include "cssbot_guard_hostage_task.h"

CCSSBotGuardHostageTask::CCSSBotGuardHostageTask(CBaseEntity* hostage) :
	m_hostage(hostage)
{
}

AITask<CCSSBot>* CCSSBotGuardHostageTask::InitialNextTask(CCSSBot* bot)
{
	CBaseEntity* hostage = m_hostage.Get();

	if (hostage)
	{
		const Vector& pos = UtilHelpers::getEntityOrigin(hostage);
		return new CBotSharedDefendSpotTask<CCSSBot, CCSSBotPathCost>(bot, pos, -1.0f, true);
	}

	return nullptr;
}

TaskResult<CCSSBot> CCSSBotGuardHostageTask::OnTaskUpdate(CCSSBot* bot)
{
	if (CCounterStrikeSourceMod::GetCSSMod()->AreTheHostagesBeingRescued())
	{
		return Done("Hostages are being rescued!");
	}

	if (GetNextTask() == nullptr)
	{
		return Done("Done camping!");
	}

	return Continue();
}
