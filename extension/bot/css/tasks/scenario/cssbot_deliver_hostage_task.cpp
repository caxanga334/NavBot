#include NAVBOT_PCH_FILE
#include <mods/modhelpers.h>
#include <mods/css/nav/css_nav_mesh.h>
#include <mods/css/css_lib.h>
#include <bot/css/cssbot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "cssbot_deliver_hostage_task.h"

TaskResult<CCSSBot> CCSSBotDeliverHostageTask::OnTaskStart(CCSSBot* bot, AITask<CCSSBot>* pastTask)
{
	CBaseEntity* rescuezone = UtilHelpers::GetRandomEntityOfClassname("func_hostage_rescue");

	if (!rescuezone)
	{
		return Done("No rescue zone available!");
	}

	m_goal = trace::getground(UtilHelpers::getWorldSpaceCenter(rescuezone));

	return Continue();
}

TaskResult<CCSSBot> CCSSBotDeliverHostageTask::OnTaskUpdate(CCSSBot* bot)
{
	if (!csslib::IsEscortingHostages(bot->GetEntity()))
	{
		return Done("No longer escorting hostages.");
	}

	if (!m_didtouch)
	{
		if (csslib::IsInHostageRescueZone(bot->GetEntity()))
		{
			// the bot just touched a hostage rescue zone but the hostages needs to touch it in order to be rescued.
			// calculate a position to move
			Vector center = m_goal; // the zone center ground position calculated before
			Vector to = UtilHelpers::math::BuildDirectionVector(bot->GetAbsOrigin(), center);
			Vector goal = center + (to * 128.0f);
			m_goal = goal;
			m_nav.Invalidate(); // force a repath
			m_didtouch = true; // only calculate this once
			m_timeout.StartRandom(5.0f, 10.0f); // fail safe timer in case hostages get stuck
		}
	}
	else
	{
		if (m_timeout.IsElapsed())
		{
			return Done("Timeout timer is elapsed!");
		}
	}

	if (!m_nav.IsValid() || m_nav.NeedsRepath())
	{
		CCSSBotPathCost cost{ bot, RouteType::SAFEST_ROUTE };
		cost.SetEscortingHostages(true);
		m_nav.ComputePathToPosition(bot, m_goal, cost);
		m_nav.StartRepathTimer();
	}

	m_nav.Update(bot);
	return Continue();
}
