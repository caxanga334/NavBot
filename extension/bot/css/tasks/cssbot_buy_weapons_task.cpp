#include NAVBOT_PCH_FILE
#include <mods/css/css_lib.h>
#include <mods/css/css_mod.h>
#include <bot/css/cssbot.h>
#include "cssbot_tactical_task.h"
#include "cssbot_buy_weapons_task.h"

TaskResult<CCSSBot> CCSSBotBuyWeaponsTask::OnTaskStart(CCSSBot* bot, AITask<CCSSBot>* pastTask)
{
	m_buydelay.StartRandom(3.0f, 7.0f);
	m_didbuy = false;

	return Continue();
}

TaskResult<CCSSBot> CCSSBotBuyWeaponsTask::OnTaskUpdate(CCSSBot* bot)
{
	if (m_buydelay.IsElapsed())
	{
		if (!m_didbuy)
		{
			CCounterStrikeSourceMod::GetCSSMod()->BuyWeapons(bot);
			m_didbuy = true;
		}

		if (!csslib::IsInFreezeTime())
		{
			bot->GetInventoryInterface()->RequestRefresh();
			return SwitchTo(new CCSSBotTacticalTask, "Freeze time over!");
		}
	}

	return Continue();
}
