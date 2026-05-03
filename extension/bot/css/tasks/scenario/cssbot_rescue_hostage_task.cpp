#include NAVBOT_PCH_FILE
#include <mods/modhelpers.h>
#include <mods/css/nav/css_nav_mesh.h>
#include <mods/css/css_lib.h>
#include <bot/css/cssbot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "cssbot_rescue_hostage_task.h"

CCSSBotRescueHostageTask::CCSSBotRescueHostageTask(CBaseEntity* hostage) :
	m_hostage(hostage)
{
}

TaskResult<CCSSBot> CCSSBotRescueHostageTask::OnTaskUpdate(CCSSBot* bot)
{
	CBaseEntity* hostage = m_hostage.Get();

	if (!hostage)
	{
		return Done("Hostage is NULL!");
	}

	if (modhelpers->IsDead(hostage) || csslib::IsHostageRescued(hostage) || csslib::GetHostageLeader(hostage) != nullptr)
	{
		CBaseEntity* next = csslib::GetRandomHostageToRescue();

		if (!next)
		{
			return Done("No more hostages to rescue!");
		}

		m_hostage = next;
		hostage = next;
		m_nav.Invalidate();
	}

	const Vector& goal = UtilHelpers::getEntityOrigin(hostage);

	if (!m_nav.IsValid() || m_nav.NeedsRepath())
	{
		CCSSBotPathCost cost{ bot, RouteType::SAFEST_ROUTE };
		cost.SetEscortingHostages(csslib::IsEscortingHostages(bot->GetEntity()));
		m_nav.ComputePathToPosition(bot, goal, cost);
		m_nav.StartRepathTimer();
	}

	m_nav.Update(bot);

	if (bot->IsDebugging(BOTDEBUG_TASKS))
	{
		NDebugOverlay::EntityBounds(hostage, 255, 255, 0, 90, 0.1f);
		NDebugOverlay::Text(UtilHelpers::getWorldSpaceCenter(hostage), true, 0.1f, "GOAL HOSTAGE: %s", bot->GetClientName());
	}

	if ((bot->GetEyeOrigin() - goal).IsLengthLessThan(CBaseExtPlayer::PLAYER_USE_RADIUS - 1.0f))
	{
		IPlayerController* control = bot->GetControlInterface();

		control->AimAt(UtilHelpers::getWorldSpaceCenter(hostage), IPlayerController::LOOK_CRITICAL, 1.0f, "Looking at hostage to use it!");

		if (control->IsAimOnTarget())
		{
			control->PressUseButton();
		}
	}

	return Continue();
}
